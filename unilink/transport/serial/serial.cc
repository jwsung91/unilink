/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "unilink/transport/serial/serial.hpp"

#include <cstring>
#include <iostream>

#include "unilink/common/io_context_manager.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/transport/serial/boost_serial_port.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

// Use fully qualified names for clarity
using namespace common;  // For error_reporting namespace

namespace {
// Ensure IoContextManager is running before we capture its io_context reference
net::io_context& acquire_shared_serial_context() {
  auto& manager = common::IoContextManager::instance();
  manager.start();
  return manager.get_context();
}
}  // namespace

Serial::Serial(const config::SerialConfig& cfg)
    : ioc_(acquire_shared_serial_context()),
      owns_ioc_(false),  // Shared global io_context managed by IoContextManager
      cfg_(cfg),
      retry_timer_(ioc_),
      bp_high_(cfg.backpressure_threshold) {
  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;

  rx_.resize(cfg_.read_chunk);
  port_ = std::make_unique<BoostSerialPort>(ioc_);
}

// For testing with dependency injection
Serial::Serial(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port, net::io_context& ioc)
    : ioc_(ioc),
      owns_ioc_(false),
      port_(std::move(port)),
      cfg_(cfg),
      retry_timer_(ioc_),
      bp_high_(cfg.backpressure_threshold) {
  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;

  rx_.resize(cfg_.read_chunk);
}

Serial::~Serial() {
  // stop() might have been called already. Ensure we don't double-stop,
  // but do clean up resources if we own them.
  if (started_ && !state_.is_state(common::LinkState::Closed)) stop();

  // No need to clean up io_context as it's shared and managed by IoContextManager
}

void Serial::start() {
  if (started_) return;
  stopping_.store(false);
  UNILINK_LOG_INFO("serial", "start", "Starting device: " + cfg_.device);
  if (!owns_ioc_) {
    auto& manager = common::IoContextManager::instance();
    if (!manager.is_running()) {
      manager.start();
    }
    if (ioc_.stopped()) {
      ioc_.restart();
    }
  }
  work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(ioc_.get_executor());
  if (owns_ioc_) {
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }
  auto self = weak_from_this();
  net::post(ioc_, [self] {
    if (auto s = self.lock()) {
      UNILINK_LOG_DEBUG("serial", "start", "Posting open_and_configure for device: " + s->cfg_.device);
      s->state_.set_state(common::LinkState::Connecting);
      s->notify_state();
      s->open_and_configure();
    }
  });
  started_ = true;
}

void Serial::stop() {
  if (!started_) {
    state_.set_state(common::LinkState::Closed);
    return;
  }

  stopping_.store(true);
  if (!state_.is_state(common::LinkState::Closed)) {
    if (work_guard_) work_guard_->reset();  // Allow the io_context to run out of work.
    auto self = weak_from_this();
    net::post(ioc_, [self] {
      if (auto s = self.lock()) {
        // Cancel all pending async operations to unblock the io_context
        s->retry_timer_.cancel();
        s->close_port();
        s->tx_.clear();
        s->queued_bytes_ = 0;
        s->writing_ = false;
        // Post stop() to ensure it's the last thing to run before the context
        // runs out of work.
        if (s->owns_ioc_) {
          s->ioc_.stop();
        }
      }
    });

    // Wait for all async operations to complete
    if (owns_ioc_ && ioc_thread_.joinable()) {
      ioc_thread_.join();
      // Reset the io_context to clear any remaining work
      ioc_.restart();
    }

    opened_.store(false);
    state_.set_state(common::LinkState::Closed);
    notify_state();
  }
  started_ = false;
}

bool Serial::is_connected() const { return opened_.load(); }

void Serial::async_write_copy(const uint8_t* data, size_t n) {
  if (stopping_.load() || state_.is_state(common::LinkState::Closed) || state_.is_state(common::LinkState::Error)) {
    return;
  }

  // Use memory pool for better performance (only for reasonable sizes)
  if (n <= 65536) {  // Only use pool for buffers <= 64KB
    common::PooledBuffer pooled_buffer(n);
    if (pooled_buffer.valid()) {
      // Copy data to pooled buffer safely
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data, n);

      net::post(ioc_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        self->queued_bytes_ += buf.size();
        self->tx_.emplace_back(std::move(buf));
        if (self->on_bp_ && self->queued_bytes_ > self->bp_high_) self->on_bp_(self->queued_bytes_);
        if (!self->writing_) self->do_write();
      });
      return;
    }
  }

  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + n);

  net::post(ioc_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    self->queued_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    if (self->on_bp_ && self->queued_bytes_ > self->bp_high_) self->on_bp_(self->queued_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void Serial::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void Serial::on_state(OnState cb) { on_state_ = std::move(cb); }
void Serial::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void Serial::open_and_configure() {
  boost::system::error_code ec;
  port_->open(cfg_.device, ec);
  if (ec) {
    UNILINK_LOG_ERROR("serial", "open", "Failed to open device: " + cfg_.device + " - " + ec.message());
    handle_error("open", ec);
    return;
  }

  port_->set_option(net::serial_port_base::baud_rate(cfg_.baud_rate), ec);
  if (ec) {
    UNILINK_LOG_ERROR("serial", "configure",
                      "Failed to set baud rate: " + std::to_string(cfg_.baud_rate) + " - " + ec.message());
    handle_error("baud_rate", ec);
    return;
  }

  port_->set_option(net::serial_port_base::character_size(cfg_.char_size), ec);
  if (ec) {
    UNILINK_LOG_ERROR("serial", "configure",
                      "Failed to set character size: " + std::to_string(cfg_.char_size) + " - " + ec.message());
    handle_error("char_size", ec);
    return;
  }

  using sb = net::serial_port_base::stop_bits;
  port_->set_option(sb(cfg_.stop_bits == 2 ? sb::two : sb::one), ec);
  if (ec) {
    UNILINK_LOG_ERROR("serial", "configure",
                      "Failed to set stop bits: " + std::to_string(cfg_.stop_bits) + " - " + ec.message());
    handle_error("stop_bits", ec);
    return;
  }

  using pa = net::serial_port_base::parity;
  pa::type p = pa::none;
  if (cfg_.parity == SerialConfig::Parity::Even)
    p = pa::even;
  else if (cfg_.parity == SerialConfig::Parity::Odd)
    p = pa::odd;
  port_->set_option(pa(p), ec);
  if (ec) {
    UNILINK_LOG_ERROR("serial", "configure", "Failed to set parity - " + ec.message());
    handle_error("parity", ec);
    return;
  }

  using fc = net::serial_port_base::flow_control;
  fc::type f = fc::none;
  if (cfg_.flow == SerialConfig::Flow::Software)
    f = fc::software;
  else if (cfg_.flow == SerialConfig::Flow::Hardware)
    f = fc::hardware;
  port_->set_option(fc(f), ec);
  if (ec) {
    UNILINK_LOG_ERROR("serial", "configure", "Failed to set flow control - " + ec.message());
    handle_error("flow_control", ec);
    return;
  }

  UNILINK_LOG_INFO("serial", "connect", "Device opened: " + cfg_.device + " @ " + std::to_string(cfg_.baud_rate));
  start_read();

  opened_.store(true);
  state_.set_state(common::LinkState::Connected);
  notify_state();
}

void Serial::start_read() {
  auto self = shared_from_this();
  port_->async_read_some(net::buffer(rx_.data(), rx_.size()), [self](auto ec, std::size_t n) {
    if (ec) {
      self->handle_error("read", ec);
      return;
    }
    if (self->on_bytes_) self->on_bytes_(self->rx_.data(), n);
    self->start_read();
  });
}

void Serial::do_write() {
  if (tx_.empty()) {
    writing_ = false;
    return;
  }
  writing_ = true;
  auto self = shared_from_this();

  // Handle both PooledBuffer and std::vector<uint8_t> (fallback)
  auto& front_buffer = tx_.front();
  if (std::holds_alternative<common::PooledBuffer>(front_buffer)) {
    auto& pooled_buf = std::get<common::PooledBuffer>(front_buffer);
    port_->async_write(net::buffer(pooled_buf.data(), pooled_buf.size()), [self](auto ec, std::size_t n) {
      self->queued_bytes_ -= n;
      if (ec) {
        self->handle_error("write", ec);
        return;
      }
      self->tx_.pop_front();
      self->do_write();
    });
  } else {
    auto& vec_buf = std::get<std::vector<uint8_t>>(front_buffer);
    port_->async_write(net::buffer(vec_buf), [self](auto ec, std::size_t n) {
      self->queued_bytes_ -= n;
      if (ec) {
        self->handle_error("write", ec);
        return;
      }
      self->tx_.pop_front();
      self->do_write();
    });
  }
}

void Serial::handle_error(const char* where, const boost::system::error_code& ec) {
  // EOF is not a real error, so restart reading
  if (ec == boost::asio::error::eof) {
    UNILINK_LOG_DEBUG("serial", "read", "EOF detected, restarting read");
    start_read();
    return;
  }

  if (stopping_.load() || ec == boost::asio::error::operation_aborted) {
    opened_.store(false);
    close_port();
    state_.set_state(common::LinkState::Closed);
    notify_state();
    return;
  }

  // 구조화된 에러 처리
  bool retryable = cfg_.reopen_on_error;
  common::error_reporting::report_connection_error("serial", where, ec, retryable);

  UNILINK_LOG_ERROR("serial", where, "Error: " + ec.message() + " (code: " + std::to_string(ec.value()) + ")");

  if (cfg_.reopen_on_error) {
    opened_.store(false);
    close_port();
    state_.set_state(common::LinkState::Connecting);
    notify_state();
    schedule_retry(where, ec);
  } else {
    opened_.store(false);
    close_port();
    state_.set_state(common::LinkState::Error);
    notify_state();
  }
}

void Serial::schedule_retry(const char* where, const boost::system::error_code& ec) {
  UNILINK_LOG_INFO("serial", "retry",
                   "Scheduling retry after " + std::to_string(cfg_.retry_interval_ms / 1000.0) + "s at " + where +
                       " (" + ec.message() + ")");
  if (stopping_.load()) return;
  auto self = shared_from_this();
  retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
  retry_timer_.async_wait([self](auto e) {
    if (!e && !self->stopping_.load()) self->open_and_configure();
  });
}

void Serial::set_retry_interval(unsigned interval_ms) { cfg_.retry_interval_ms = interval_ms; }

void Serial::close_port() {
  boost::system::error_code ec;
  if (port_ && port_->is_open()) {
    // For serial_port, close() cancels pending asynchronous operations. The
    // read handler will be called with an error.
    port_->close(ec);
  }
}

void Serial::notify_state() {
  if (on_state_) {
    try {
      on_state_(state_.get_state());
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("serial", "callback", "State callback error: " + std::string(e.what()));
      common::error_reporting::report_system_error("serial", "state_callback",
                                                   "Exception in state callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("serial", "callback", "Unknown error in state callback");
      common::error_reporting::report_system_error("serial", "state_callback", "Unknown error in state callback");
    }
  }
}

}  // namespace transport
}  // namespace unilink
