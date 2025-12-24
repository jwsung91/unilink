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

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg) {
  return std::shared_ptr<Serial>(new Serial(cfg));
}

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg, net::io_context& ioc) {
  return std::shared_ptr<Serial>(new Serial(cfg, std::make_unique<BoostSerialPort>(ioc), ioc));
}

std::shared_ptr<Serial> Serial::create(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port,
                                       net::io_context& ioc) {
  return std::shared_ptr<Serial>(new Serial(cfg, std::move(port), ioc));
}

Serial::Serial(const config::SerialConfig& cfg)
    : ioc_(acquire_shared_serial_context()),
      owns_ioc_(false),  // Shared global io_context managed by IoContextManager
      strand_(ioc_.get_executor()),
      cfg_(cfg),
      retry_timer_(ioc_),
      bp_high_(cfg.backpressure_threshold) {
  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) bp_low_ = 1;

  rx_.resize(cfg_.read_chunk);
  port_ = std::make_unique<BoostSerialPort>(ioc_);
}

// For testing with dependency injection
Serial::Serial(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port, net::io_context& ioc)
    : ioc_(ioc),
      owns_ioc_(false),
      strand_(ioc_.get_executor()),
      port_(std::move(port)),
      cfg_(cfg),
      retry_timer_(ioc_),
      bp_high_(cfg.backpressure_threshold) {
  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) bp_low_ = 1;

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
  net::post(strand_, [self] {
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
    net::post(strand_, [self] {
      if (auto s = self.lock()) {
        // Cancel all pending async operations to unblock the io_context
        s->retry_timer_.cancel();
        s->close_port();
        s->tx_.clear();
        s->queued_bytes_ = 0;
        s->writing_ = false;
        s->report_backpressure(s->queued_bytes_);
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

  if (n > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum allowed");
    return;
  }

  // Use memory pool for better performance (only for reasonable sizes)
  if (n <= 65536) {  // Only use pool for buffers <= 64KB
    common::PooledBuffer pooled_buffer(n);
    if (pooled_buffer.valid()) {
      // Copy data to pooled buffer safely
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data, n);

      net::post(strand_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        if (self->queued_bytes_ + buf.size() > self->bp_limit_) {
          UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
          self->opened_.store(false);
          self->close_port();
          self->tx_.clear();
          self->queued_bytes_ = 0;
          self->writing_ = false;
          self->state_.set_state(common::LinkState::Error);
          self->notify_state();
          self->report_backpressure(self->queued_bytes_);
          return;
        }
        self->queued_bytes_ += buf.size();
        self->tx_.emplace_back(std::move(buf));
        self->report_backpressure(self->queued_bytes_);
        if (!self->writing_) self->do_write();
      });
      return;
    }
  }

  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + n);

  net::post(strand_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    if (self->queued_bytes_ + buf.size() > self->bp_limit_) {
      UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
      self->opened_.store(false);
      self->close_port();
      self->tx_.clear();
      self->queued_bytes_ = 0;
      self->writing_ = false;
      self->state_.set_state(common::LinkState::Error);
      self->notify_state();
      self->report_backpressure(self->queued_bytes_);
      return;
    }
    self->queued_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queued_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void Serial::async_write_move(std::vector<uint8_t>&& data) {
  if (stopping_.load() || state_.is_state(common::LinkState::Closed) || state_.is_state(common::LinkState::Error)) {
    return;
  }
  const auto added = data.size();
  if (added > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum allowed");
    return;
  }
  net::post(strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->queued_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
      self->opened_.store(false);
      self->close_port();
      self->tx_.clear();
      self->queued_bytes_ = 0;
      self->writing_ = false;
      self->state_.set_state(common::LinkState::Error);
      self->notify_state();
      self->report_backpressure(self->queued_bytes_);
      return;
    }
    self->queued_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queued_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void Serial::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (stopping_.load() || state_.is_state(common::LinkState::Closed) || state_.is_state(common::LinkState::Error)) {
    return;
  }
  if (!data || data->empty()) return;
  const auto added = data->size();
  if (added > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum allowed");
    return;
  }
  net::post(strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->queued_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
      self->opened_.store(false);
      self->close_port();
      self->tx_.clear();
      self->queued_bytes_ = 0;
      self->writing_ = false;
      self->state_.set_state(common::LinkState::Error);
      self->notify_state();
      self->report_backpressure(self->queued_bytes_);
      return;
    }
    self->queued_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queued_bytes_);
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

  // Flush any pending writes that were queued during reconnection
  do_write();
}

void Serial::start_read() {
  auto self = shared_from_this();
  port_->async_read_some(net::buffer(rx_.data(), rx_.size()),
                         net::bind_executor(strand_, [self](auto ec, std::size_t n) {
                           if (ec) {
                             self->handle_error("read", ec);
                             return;
                           }
                           if (self->on_bytes_) self->on_bytes_(self->rx_.data(), n);
                           self->start_read();
                         }));
}

void Serial::do_write() {
  if (tx_.empty()) {
    writing_ = false;
    return;
  }
  writing_ = true;
  auto self = shared_from_this();

  // Move buffer out of queue immediately to ensure lifetime safety during async op
  auto current = std::move(tx_.front());
  tx_.pop_front();

  auto on_write = [self](const boost::system::error_code& ec, std::size_t n) {
    if (self->queued_bytes_ >= n) {
      self->queued_bytes_ -= n;
    } else {
      self->queued_bytes_ = 0;
    }
    self->report_backpressure(self->queued_bytes_);

    if (ec) {
      self->handle_error("write", ec);
      return;
    }
    self->do_write();
  };

  if (std::holds_alternative<common::PooledBuffer>(current)) {
    auto pooled_buf = std::get<common::PooledBuffer>(std::move(current));
    auto shared_pooled = std::make_shared<common::PooledBuffer>(std::move(pooled_buf));
    auto data = shared_pooled->data();
    auto size = shared_pooled->size();
    port_->async_write(net::buffer(data, size), net::bind_executor(strand_, [self, buf = shared_pooled, on_write](
                                                                                auto ec, auto n) { on_write(ec, n); }));
  } else if (std::holds_alternative<std::shared_ptr<const std::vector<uint8_t>>>(current)) {
    auto shared_buf = std::get<std::shared_ptr<const std::vector<uint8_t>>>(std::move(current));
    auto data = shared_buf->data();
    auto size = shared_buf->size();
    port_->async_write(net::buffer(data, size),
                       net::bind_executor(strand_, [self, buf = std::move(shared_buf), on_write](auto ec, auto n) {
                         on_write(ec, n);
                       }));
  } else {
    auto vec_buf = std::get<std::vector<uint8_t>>(std::move(current));
    auto shared_vec = std::make_shared<std::vector<uint8_t>>(std::move(vec_buf));
    auto data = shared_vec->data();
    auto size = shared_vec->size();
    port_->async_write(net::buffer(data, size), net::bind_executor(strand_, [self, buf = shared_vec, on_write](
                                                                                auto ec, auto n) { on_write(ec, n); }));
  }
}

void Serial::handle_error(const char* where, const boost::system::error_code& ec) {
  // EOF is not a real error, so restart reading
  if (ec == boost::asio::error::eof) {
    UNILINK_LOG_DEBUG("serial", "read", "EOF detected, restarting read");
    start_read();
    return;
  }

  if (stopping_.load()) {
    opened_.store(false);
    close_port();
    state_.set_state(common::LinkState::Closed);
    notify_state();
    return;
  }

  if (ec == boost::asio::error::operation_aborted) {
    // If we already flagged an error (e.g., queue overflow) don't override with Closed.
    if (state_.is_state(common::LinkState::Error)) {
      return;
    }
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

void Serial::report_backpressure(size_t queued_bytes) {
  if (!on_bp_) return;
  if (!backpressure_active_ && queued_bytes >= bp_high_) {
    backpressure_active_ = true;
    on_bp_(queued_bytes);
  } else if (backpressure_active_ && queued_bytes <= bp_low_) {
    backpressure_active_ = false;
    on_bp_(queued_bytes);
  }
}

}  // namespace transport
}  // namespace unilink
