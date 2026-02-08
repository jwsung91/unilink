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

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <variant>

#include "unilink/base/constants.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/transport/serial/boost_serial_port.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using namespace common;

namespace {
net::io_context& acquire_shared_serial_context() {
  auto& manager = concurrency::IoContextManager::instance();
  manager.start();
  return manager.get_context();
}
}  // namespace

struct Serial::Impl {
  net::io_context& ioc_;
  bool owns_ioc_;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::SerialPortInterface> port_;
  SerialConfig cfg_;
  net::steady_timer retry_timer_;

  std::vector<uint8_t> rx_;

  using BufferVariant =
      std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>;
  std::deque<BufferVariant> tx_;
  std::optional<BufferVariant> current_write_buffer_;

  bool writing_ = false;
  size_t queued_bytes_ = 0;
  size_t bp_high_;
  size_t bp_limit_;
  size_t bp_low_;
  bool backpressure_active_ = false;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  std::atomic<bool> opened_{false};
  concurrency::ThreadSafeLinkState state_{base::LinkState::Idle};

  std::atomic<bool> started_{false};
  std::atomic<bool> stopping_{false};

  Impl(const SerialConfig& cfg, net::io_context& ioc, bool owns_ioc,
       std::unique_ptr<interface::SerialPortInterface> port)
      : ioc_(ioc),
        owns_ioc_(owns_ioc),
        strand_(ioc_.get_executor()),
        port_(std::move(port)),
        cfg_(cfg),
        retry_timer_(ioc_),
        bp_high_(cfg.backpressure_threshold) {
    if (!port_) {
      port_ = std::make_unique<BoostSerialPort>(ioc_);
    }
    cfg_.validate_and_clamp();
    bp_high_ = cfg_.backpressure_threshold;
    bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                         common::constants::MAX_BUFFER_SIZE);
    bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
    if (bp_low_ == 0) bp_low_ = 1;

    rx_.resize(cfg_.read_chunk);
  }

  void start(std::shared_ptr<Serial> self) {
    if (started_) return;
    stopping_.store(false);
    UNILINK_LOG_INFO("serial", "start", "Starting device: " + cfg_.device);
    if (!owns_ioc_) {
      auto& manager = concurrency::IoContextManager::instance();
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
    net::post(strand_, [this, self] {
      if (!stopping_.load()) {
        UNILINK_LOG_DEBUG("serial", "start", "Posting open_and_configure for device: " + cfg_.device);
        state_.set_state(base::LinkState::Connecting);
        notify_state();
        open_and_configure(self);
      }
    });
    started_ = true;
  }

  void stop(std::shared_ptr<Serial> self) {
    if (!started_) {
      state_.set_state(base::LinkState::Closed);
      return;
    }

    stopping_.store(true);
    if (!state_.is_state(base::LinkState::Closed)) {
      if (work_guard_) work_guard_->reset();
      net::post(strand_, [this, self] {
        retry_timer_.cancel();
        close_port();
        tx_.clear();
        queued_bytes_ = 0;
        writing_ = false;
        report_backpressure(queued_bytes_);
        if (owns_ioc_) {
          ioc_.stop();
        }
      });

      if (owns_ioc_ && ioc_thread_.joinable()) {
        ioc_thread_.join();
        ioc_.restart();
      }

      opened_.store(false);
      state_.set_state(base::LinkState::Closed);
      notify_state();
    }
    started_ = false;
  }

  void open_and_configure(std::shared_ptr<Serial> self) {
    boost::system::error_code ec;
    port_->open(cfg_.device, ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "open", "Failed to open device: " + cfg_.device + " - " + ec.message());
      handle_error(self, "open", ec);
      return;
    }

    port_->set_option(net::serial_port_base::baud_rate(cfg_.baud_rate), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure",
                        "Failed to set baud rate: " + std::to_string(cfg_.baud_rate) + " - " + ec.message());
      handle_error(self, "baud_rate", ec);
      return;
    }

    port_->set_option(net::serial_port_base::character_size(cfg_.char_size), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure",
                        "Failed to set character size: " + std::to_string(cfg_.char_size) + " - " + ec.message());
      handle_error(self, "char_size", ec);
      return;
    }

    using sb = net::serial_port_base::stop_bits;
    port_->set_option(sb(cfg_.stop_bits == 2 ? sb::two : sb::one), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure",
                        "Failed to set stop bits: " + std::to_string(cfg_.stop_bits) + " - " + ec.message());
      handle_error(self, "stop_bits", ec);
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
      handle_error(self, "parity", ec);
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
      handle_error(self, "flow_control", ec);
      return;
    }

    UNILINK_LOG_INFO("serial", "connect", "Device opened: " + cfg_.device + " @ " + std::to_string(cfg_.baud_rate));
    start_read(self);

    opened_.store(true);
    state_.set_state(base::LinkState::Connected);
    notify_state();

    do_write(self);
  }

  void start_read(std::shared_ptr<Serial> self) {
    port_->async_read_some(
        net::buffer(rx_.data(), rx_.size()), net::bind_executor(strand_, [this, self](auto ec, std::size_t n) {
          if (ec) {
            handle_error(self, "read", ec);
            return;
          }
          if (on_bytes_) {
            try {
              on_bytes_(memory::ConstByteSpan(rx_.data(), n));
            } catch (const std::exception& e) {
              UNILINK_LOG_ERROR("serial", "on_bytes", "Exception in on_bytes callback: " + std::string(e.what()));
              if (cfg_.stop_on_callback_exception) {
                opened_.store(false);
                close_port();
                state_.set_state(base::LinkState::Error);
                notify_state();
                return;
              }
              handle_error(self, "on_bytes_callback", make_error_code(boost::system::errc::io_error));
              return;
            } catch (...) {
              UNILINK_LOG_ERROR("serial", "on_bytes", "Unknown exception in on_bytes callback");
              if (cfg_.stop_on_callback_exception) {
                opened_.store(false);
                close_port();
                state_.set_state(base::LinkState::Error);
                notify_state();
                return;
              }
              handle_error(self, "on_bytes_callback", make_error_code(boost::system::errc::io_error));
              return;
            }
          }
          start_read(self);
        }));
  }

  void do_write(std::shared_ptr<Serial> self) {
    if (stopping_.load() || tx_.empty()) {
      writing_ = false;
      return;
    }
    writing_ = true;

    current_write_buffer_ = std::move(tx_.front());
    tx_.pop_front();

    auto& current = *current_write_buffer_;

    auto on_write = [this, self](const boost::system::error_code& ec, std::size_t n) {
      current_write_buffer_.reset();

      if (queued_bytes_ >= n) {
        queued_bytes_ -= n;
      } else {
        queued_bytes_ = 0;
      }
      report_backpressure(queued_bytes_);

      if (stopping_.load()) {
        writing_ = false;
        return;
      }

      if (ec) {
        handle_error(self, "write", ec);
        return;
      }
      do_write(self);
    };

    std::visit(
        [this, &on_write](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, memory::PooledBuffer>) {
            port_->async_write(net::buffer(arg.data(), arg.size()), net::bind_executor(strand_, on_write));
          } else if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
            port_->async_write(net::buffer(arg->data(), arg->size()), net::bind_executor(strand_, on_write));
          } else {
            port_->async_write(net::buffer(arg.data(), arg.size()), net::bind_executor(strand_, on_write));
          }
        },
        current);
  }

  void handle_error(std::shared_ptr<Serial> self, const char* where, const boost::system::error_code& ec) {
    if (ec == net::error::eof) {
      UNILINK_LOG_DEBUG("serial", "read", "EOF detected, restarting read");
      start_read(self);
      return;
    }

    if (stopping_.load()) {
      opened_.store(false);
      close_port();
      state_.set_state(base::LinkState::Closed);
      notify_state();
      return;
    }

    if (ec == net::error::operation_aborted) {
      if (state_.is_state(base::LinkState::Error)) {
        return;
      }
      opened_.store(false);
      close_port();
      state_.set_state(base::LinkState::Closed);
      notify_state();
      return;
    }

    bool retryable = cfg_.reopen_on_error;
    diagnostics::error_reporting::report_connection_error("serial", where, ec, retryable);

    UNILINK_LOG_ERROR("serial", where, "Error: " + ec.message() + " (code: " + std::to_string(ec.value()) + ")");

    if (cfg_.reopen_on_error) {
      opened_.store(false);
      close_port();
      state_.set_state(base::LinkState::Connecting);
      notify_state();
      schedule_retry(self, where, ec);
    } else {
      opened_.store(false);
      close_port();
      state_.set_state(base::LinkState::Error);
      notify_state();
    }
  }

  void schedule_retry(std::shared_ptr<Serial> self, const char* where, const boost::system::error_code& ec) {
    UNILINK_LOG_INFO("serial", "retry",
                     "Scheduling retry after " + std::to_string(cfg_.retry_interval_ms / 1000.0) + "s at " + where +
                         " (" + ec.message() + ")");
    if (stopping_.load()) return;
    retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
    retry_timer_.async_wait([this, self](auto e) {
      if (!e && !stopping_.load()) open_and_configure(self);
    });
  }

  void close_port() {
    boost::system::error_code ec;
    if (port_ && port_->is_open()) {
      port_->close(ec);
    }
  }

  void report_backpressure(size_t queued_bytes) {
    if (stopping_.load() || !on_bp_) return;

    if (!backpressure_active_ && queued_bytes >= bp_high_) {
      backpressure_active_ = true;
      try {
        on_bp_(queued_bytes);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("serial", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
      }
    } else if (backpressure_active_ && queued_bytes <= bp_low_) {
      backpressure_active_ = false;
      try {
        on_bp_(queued_bytes);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("serial", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
      }
    }
  }

  void notify_state() {
    if (stopping_.load() || !on_state_) return;
    try {
      on_state_(state_.get_state());
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("serial", "callback", "State callback error: " + std::string(e.what()));
    }
  }
};

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg) {
  return std::shared_ptr<Serial>(new Serial(cfg));
}

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg, boost::asio::io_context& ioc) {
  return std::shared_ptr<Serial>(new Serial(cfg, nullptr, ioc));
}

std::shared_ptr<Serial> Serial::create(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port,
                                       boost::asio::io_context& ioc) {
  return std::shared_ptr<Serial>(new Serial(cfg, std::move(port), ioc));
}

Serial::Serial(const config::SerialConfig& cfg)
    : impl_(std::make_unique<Impl>(cfg, acquire_shared_serial_context(), false, nullptr)) {}

Serial::Serial(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port,
               boost::asio::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, ioc, false, std::move(port))) {}

Serial::~Serial() = default;

void Serial::start() { impl_->start(shared_from_this()); }

void Serial::stop() { impl_->stop(shared_from_this()); }

bool Serial::is_connected() const { return impl_->opened_.load(); }

void Serial::on_bytes(OnBytes cb) { impl_->on_bytes_ = std::move(cb); }

void Serial::on_state(OnState cb) { impl_->on_state_ = std::move(cb); }

void Serial::on_backpressure(OnBackpressure cb) { impl_->on_bp_ = std::move(cb); }

void Serial::set_retry_interval(unsigned interval_ms) { impl_->cfg_.retry_interval_ms = interval_ms; }

void Serial::async_write_copy(memory::ConstByteSpan data) {
  if (impl_->stopping_.load() || impl_->state_.is_state(base::LinkState::Closed) ||
      impl_->state_.is_state(base::LinkState::Error)) {
    return;
  }

  size_t n = data.size();
  if (n > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum allowed");
    return;
  }

  if (n <= 65536) {
    memory::PooledBuffer pooled_buffer(n);
    if (pooled_buffer.valid()) {
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data.data(), n);
      net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        if (self->impl_->queued_bytes_ + buf.size() > self->impl_->bp_limit_) {
          UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
          self->impl_->tx_.clear();
          self->impl_->queued_bytes_ = 0;
          self->impl_->writing_ = false;
          self->impl_->report_backpressure(self->impl_->queued_bytes_);
          self->impl_->state_.set_state(base::LinkState::Error);
          self->impl_->notify_state();
          self->impl_->handle_error(self, "write_queue_overflow",
                                    make_error_code(boost::system::errc::no_buffer_space));
          return;
        }
        self->impl_->queued_bytes_ += buf.size();
        self->impl_->tx_.emplace_back(std::move(buf));
        self->impl_->report_backpressure(self->impl_->queued_bytes_);
        if (!self->impl_->writing_) self->impl_->do_write(self);
      });
      return;
    }
  }

  std::vector<uint8_t> fallback(data.begin(), data.end());
  net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    if (self->impl_->queued_bytes_ + buf.size() > self->impl_->bp_limit_) {
      UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
      self->impl_->tx_.clear();
      self->impl_->queued_bytes_ = 0;
      self->impl_->writing_ = false;
      self->impl_->report_backpressure(self->impl_->queued_bytes_);
      self->impl_->state_.set_state(base::LinkState::Error);
      self->impl_->notify_state();
      self->impl_->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
      return;
    }
    self->impl_->queued_bytes_ += buf.size();
    self->impl_->tx_.emplace_back(std::move(buf));
    self->impl_->report_backpressure(self->impl_->queued_bytes_);
    if (!self->impl_->writing_) self->impl_->do_write(self);
  });
}

void Serial::async_write_move(std::vector<uint8_t>&& data) {
  if (impl_->stopping_.load() || impl_->state_.is_state(base::LinkState::Closed) ||
      impl_->state_.is_state(base::LinkState::Error)) {
    return;
  }
  const auto added = data.size();
  if (added > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum allowed");
    return;
  }
  net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->impl_->queued_bytes_ + added > self->impl_->bp_limit_) {
      UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
      self->impl_->tx_.clear();
      self->impl_->queued_bytes_ = 0;
      self->impl_->writing_ = false;
      self->impl_->report_backpressure(self->impl_->queued_bytes_);
      self->impl_->state_.set_state(base::LinkState::Error);
      self->impl_->notify_state();
      self->impl_->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
      return;
    }
    self->impl_->queued_bytes_ += added;
    self->impl_->tx_.emplace_back(std::move(buf));
    self->impl_->report_backpressure(self->impl_->queued_bytes_);
    if (!self->impl_->writing_) self->impl_->do_write(self);
  });
}

void Serial::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (impl_->stopping_.load() || impl_->state_.is_state(base::LinkState::Closed) ||
      impl_->state_.is_state(base::LinkState::Error)) {
    return;
  }
  if (!data || data->empty()) return;
  const auto added = data->size();
  if (added > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum allowed");
    return;
  }
  net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->impl_->queued_bytes_ + added > self->impl_->bp_limit_) {
      UNILINK_LOG_ERROR("serial", "write", "Queue limit exceeded");
      self->impl_->tx_.clear();
      self->impl_->queued_bytes_ = 0;
      self->impl_->writing_ = false;
      self->impl_->report_backpressure(self->impl_->queued_bytes_);
      self->impl_->state_.set_state(base::LinkState::Error);
      self->impl_->notify_state();
      self->impl_->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
      return;
    }
    self->impl_->queued_bytes_ += added;
    self->impl_->tx_.emplace_back(std::move(buf));
    self->impl_->report_backpressure(self->impl_->queued_bytes_);
    if (!self->impl_->writing_) self->impl_->do_write(self);
  });
}

}  // namespace transport
}  // namespace unilink
