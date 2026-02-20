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

#include <atomic>
#include <boost/asio.hpp>
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/base/constants.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/interface/iserial_port.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/transport/serial/boost_serial_port.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using base::LinkState;
using concurrency::ThreadSafeLinkState;

using BufferVariant =
    std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>;

namespace {
net::io_context& acquire_shared_serial_context() {
  auto& manager = concurrency::IoContextManager::instance();
  manager.start();
  return manager.get_context();
}
}  // namespace

struct Serial::Impl {
  bool started_ = false;
  std::atomic<bool> stopping_{false};
  net::io_context& ioc_;
  bool owns_ioc_{false};
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::SerialPortInterface> port_;
  config::SerialConfig cfg_;
  net::steady_timer retry_timer_;

  std::vector<uint8_t> rx_;
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
  ThreadSafeLinkState state_{LinkState::Idle};

  explicit Impl(const config::SerialConfig& cfg)
      : ioc_(acquire_shared_serial_context()),
        owns_ioc_(false),
        strand_(ioc_.get_executor()),
        cfg_(cfg),
        retry_timer_(ioc_),
        bp_high_(cfg.backpressure_threshold) {
    init();
    port_ = std::make_unique<BoostSerialPort>(ioc_);
  }

  Impl(const config::SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port, net::io_context& ioc)
      : ioc_(ioc),
        owns_ioc_(false),
        strand_(ioc.get_executor()),
        port_(std::move(port)),
        cfg_(cfg),
        retry_timer_(ioc),
        bp_high_(cfg.backpressure_threshold) {
    init();
  }

  void init() {
    cfg_.validate_and_clamp();
    bp_high_ = cfg_.backpressure_threshold;
    bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                         common::constants::MAX_BUFFER_SIZE);
    bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
    if (bp_low_ == 0) bp_low_ = 1;
    rx_.resize(cfg_.read_chunk);
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
      UNILINK_LOG_ERROR("serial", "configure", "Failed baud rate: " + ec.message());
      handle_error(self, "baud_rate", ec);
      return;
    }

    port_->set_option(net::serial_port_base::character_size(cfg_.char_size), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure", "Failed char size: " + ec.message());
      handle_error(self, "char_size", ec);
      return;
    }

    using sb = net::serial_port_base::stop_bits;
    port_->set_option(sb(cfg_.stop_bits == 2 ? sb::two : sb::one), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure", "Failed stop bits: " + ec.message());
      handle_error(self, "stop_bits", ec);
      return;
    }

    using pa = net::serial_port_base::parity;
    pa::type p = pa::none;
    if (cfg_.parity == config::SerialConfig::Parity::Even)
      p = pa::even;
    else if (cfg_.parity == config::SerialConfig::Parity::Odd)
      p = pa::odd;
    port_->set_option(pa(p), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure", "Failed parity: " + ec.message());
      handle_error(self, "parity", ec);
      return;
    }

    using fc = net::serial_port_base::flow_control;
    fc::type f = fc::none;
    if (cfg_.flow == config::SerialConfig::Flow::Software)
      f = fc::software;
    else if (cfg_.flow == config::SerialConfig::Flow::Hardware)
      f = fc::hardware;
    port_->set_option(fc(f), ec);
    if (ec) {
      UNILINK_LOG_ERROR("serial", "configure", "Failed flow control: " + ec.message());
      handle_error(self, "flow_control", ec);
      return;
    }

    UNILINK_LOG_INFO("serial", "connect", "Device opened: " + cfg_.device);
    start_read(self);

    opened_.store(true);
    state_.set_state(LinkState::Connected);
    notify_state();
    do_write(self);
  }

  void start_read(std::shared_ptr<Serial> self) {
    port_->async_read_some(
        net::buffer(rx_.data(), rx_.size()), net::bind_executor(strand_, [self](auto ec, std::size_t n) {
          auto impl = self->get_impl();
          if (ec) {
            impl->handle_error(self, "read", ec);
            return;
          }
          if (impl->on_bytes_) {
            try {
              impl->on_bytes_(memory::ConstByteSpan(impl->rx_.data(), n));
            } catch (const std::exception& e) {
              UNILINK_LOG_ERROR("serial", "on_bytes", "Exception in callback: " + std::string(e.what()));
              if (impl->cfg_.stop_on_callback_exception) {
                impl->opened_.store(false);
                impl->close_port();
                impl->state_.set_state(LinkState::Error);
                impl->notify_state();
                return;
              }
              impl->handle_error(self, "on_bytes_callback", make_error_code(boost::system::errc::io_error));
              return;
            } catch (...) {
              if (impl->cfg_.stop_on_callback_exception) {
                impl->opened_.store(false);
                impl->close_port();
                impl->state_.set_state(LinkState::Error);
                impl->notify_state();
                return;
              }
              impl->handle_error(self, "on_bytes_callback", make_error_code(boost::system::errc::io_error));
              return;
            }
          }
          impl->start_read(self);
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

    auto on_write = [self](const boost::system::error_code& ec, std::size_t n) {
      auto impl = self->get_impl();
      impl->current_write_buffer_.reset();

      if (impl->queued_bytes_ >= n) {
        impl->queued_bytes_ -= n;
      } else {
        impl->queued_bytes_ = 0;
      }
      impl->report_backpressure(impl->queued_bytes_);

      if (impl->stopping_.load()) {
        impl->writing_ = false;
        return;
      }

      if (ec) {
        impl->handle_error(self, "write", ec);
        return;
      }
      impl->do_write(self);
    };

    std::visit(
        [&](auto&& buf) {
          using T = std::decay_t<decltype(buf)>;
          auto* data_ptr = [&]() {
            if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>)
              return buf->data();
            else
              return buf.data();
          }();
          auto size = [&]() {
            if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>)
              return buf->size();
            else
              return buf.size();
          }();
          port_->async_write(net::buffer(data_ptr, size), net::bind_executor(strand_, on_write));
        },
        current);
  }

  void handle_error(std::shared_ptr<Serial> self, const char* where, const boost::system::error_code& ec) {
    if (ec == boost::asio::error::eof) {
      start_read(self);
      return;
    }

    if (stopping_.load()) {
      opened_.store(false);
      close_port();
      state_.set_state(LinkState::Closed);
      notify_state();
      return;
    }

    if (ec == boost::asio::error::operation_aborted) {
      if (state_.is_state(LinkState::Error)) return;
      opened_.store(false);
      close_port();
      state_.set_state(LinkState::Closed);
      notify_state();
      return;
    }

    bool retryable = cfg_.reopen_on_error;
    diagnostics::error_reporting::report_connection_error("serial", where, ec, retryable);

    UNILINK_LOG_ERROR("serial", where, "Error: " + ec.message());

    if (cfg_.reopen_on_error) {
      opened_.store(false);
      close_port();
      state_.set_state(LinkState::Connecting);
      notify_state();
      schedule_retry(self, where, ec);
    } else {
      opened_.store(false);
      close_port();
      state_.set_state(LinkState::Error);
      notify_state();
    }
  }

  void schedule_retry(std::shared_ptr<Serial> self, const char* where, const boost::system::error_code& ec) {
    (void)ec;
    UNILINK_LOG_INFO("serial", "retry", "Scheduling retry at " + std::string(where));
    if (stopping_.load()) return;
    retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
    retry_timer_.async_wait([self](auto e) {
      if (!e && !self->get_impl()->stopping_.load()) self->get_impl()->open_and_configure(self);
    });
  }

  void close_port() {
    boost::system::error_code ec;
    if (port_ && port_->is_open()) {
      port_->close(ec);
    }
  }

  void notify_state() {
    if (stopping_.load() || !on_state_) return;
    try {
      on_state_(state_.get_state());
    } catch (...) {
    }
  }

  void report_backpressure(size_t qb) {
    if (stopping_.load() || !on_bp_) return;

    if (!backpressure_active_ && qb >= bp_high_) {
      backpressure_active_ = true;
      try {
        on_bp_(qb);
      } catch (...) {
      }
    } else if (backpressure_active_ && qb <= bp_low_) {
      backpressure_active_ = false;
      try {
        on_bp_(qb);
      } catch (...) {
      }
    }
  }
};

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg) {
  return std::shared_ptr<Serial>(new Serial(cfg));
}

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg, net::io_context& ioc) {
  return std::shared_ptr<Serial>(new Serial(cfg, std::make_unique<BoostSerialPort>(ioc), ioc));
}

std::shared_ptr<Serial> Serial::create(const config::SerialConfig& cfg,
                                       std::unique_ptr<interface::SerialPortInterface> port, net::io_context& ioc) {
  return std::shared_ptr<Serial>(new Serial(cfg, std::move(port), ioc));
}

Serial::Serial(const config::SerialConfig& cfg) : impl_(std::make_unique<Impl>(cfg)) {}

Serial::Serial(const config::SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port,
               net::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, std::move(port), ioc)) {}

Serial::~Serial() {
  if (get_impl()->started_ && !get_impl()->state_.is_state(LinkState::Closed)) stop();
}

Serial::Serial(Serial&&) noexcept = default;
Serial& Serial::operator=(Serial&&) noexcept = default;

void Serial::start() {
  auto impl = get_impl();
  if (impl->started_) return;
  impl->stopping_.store(false);
  UNILINK_LOG_INFO("serial", "start", "Starting device: " + impl->cfg_.device);
  if (!impl->owns_ioc_) {
    auto& manager = concurrency::IoContextManager::instance();
    if (!manager.is_running()) manager.start();
    if (impl->ioc_.stopped()) impl->ioc_.restart();
  }
  impl->work_guard_ =
      std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(impl->ioc_.get_executor());
  if (impl->owns_ioc_) {
    impl->ioc_thread_ = std::thread([impl] { impl->ioc_.run(); });
  }
  auto self = shared_from_this();
  net::post(impl->strand_, [self] {
    auto impl = self->get_impl();
    impl->state_.set_state(LinkState::Connecting);
    impl->notify_state();
    impl->open_and_configure(self);
  });
  impl->started_ = true;
}

void Serial::stop() {
  auto impl = get_impl();
  if (!impl->started_) {
    impl->state_.set_state(LinkState::Closed);
    return;
  }

  impl->stopping_.store(true);
  if (!impl->state_.is_state(LinkState::Closed)) {
    if (impl->work_guard_) impl->work_guard_->reset();
    auto self = shared_from_this();
    net::post(impl->strand_, [self] {
      auto impl = self->get_impl();
      impl->retry_timer_.cancel();
      impl->close_port();
      impl->tx_.clear();
      impl->queued_bytes_ = 0;
      impl->writing_ = false;
      impl->report_backpressure(impl->queued_bytes_);
      if (impl->owns_ioc_) impl->ioc_.stop();
    });

    if (impl->owns_ioc_ && impl->ioc_thread_.joinable()) {
      impl->ioc_thread_.join();
      impl->ioc_.restart();
    }

    impl->opened_.store(false);
    impl->state_.set_state(LinkState::Closed);
    impl->notify_state();
  }
  impl->started_ = false;
}

bool Serial::is_connected() const { return get_impl()->opened_.load(); }

void Serial::async_write_copy(memory::ConstByteSpan data) {
  auto impl = get_impl();
  if (impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) || impl->state_.is_state(LinkState::Error))
    return;

  size_t n = data.size();
  if (n > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("serial", "write", "Write size exceeds maximum");
    return;
  }

  if (n <= 65536) {
    memory::PooledBuffer pooled(n);
    if (pooled.valid()) {
      common::safe_memory::safe_memcpy(pooled.data(), data.data(), n);
      net::post(impl->strand_, [self = shared_from_this(), buf = std::move(pooled)]() mutable {
        auto impl = self->get_impl();
        if (impl->queued_bytes_ + buf.size() > impl->bp_limit_) {
          impl->tx_.clear();
          impl->queued_bytes_ = 0;
          impl->writing_ = false;
          impl->report_backpressure(impl->queued_bytes_);
          impl->state_.set_state(LinkState::Error);
          impl->notify_state();
          impl->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
          return;
        }
        impl->queued_bytes_ += buf.size();
        impl->tx_.emplace_back(std::move(buf));
        impl->report_backpressure(impl->queued_bytes_);
        if (!impl->writing_) impl->do_write(self);
      });
      return;
    }
  }

  std::vector<uint8_t> fallback(data.begin(), data.end());
  net::post(impl->strand_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    auto impl = self->get_impl();
    if (impl->queued_bytes_ + buf.size() > impl->bp_limit_) {
      impl->tx_.clear();
      impl->queued_bytes_ = 0;
      impl->writing_ = false;
      impl->report_backpressure(impl->queued_bytes_);
      impl->state_.set_state(LinkState::Error);
      impl->notify_state();
      impl->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
      return;
    }
    impl->queued_bytes_ += buf.size();
    impl->tx_.emplace_back(std::move(buf));
    impl->report_backpressure(impl->queued_bytes_);
    if (!impl->writing_) impl->do_write(self);
  });
}

void Serial::async_write_move(std::vector<uint8_t>&& data) {
  auto impl = get_impl();
  if (impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) || impl->state_.is_state(LinkState::Error))
    return;
  const auto added = data.size();
  net::post(impl->strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    auto impl = self->get_impl();
    if (impl->queued_bytes_ + added > impl->bp_limit_) {
      impl->tx_.clear();
      impl->queued_bytes_ = 0;
      impl->writing_ = false;
      impl->report_backpressure(impl->queued_bytes_);
      impl->state_.set_state(LinkState::Error);
      impl->notify_state();
      impl->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
      return;
    }
    impl->queued_bytes_ += added;
    impl->tx_.emplace_back(std::move(buf));
    impl->report_backpressure(impl->queued_bytes_);
    if (!impl->writing_) impl->do_write(self);
  });
}

void Serial::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  auto impl = get_impl();
  if (impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) || impl->state_.is_state(LinkState::Error))
    return;
  if (!data || data->empty()) return;
  const auto added = data->size();
  net::post(impl->strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    auto impl = self->get_impl();
    if (impl->queued_bytes_ + added > impl->bp_limit_) {
      impl->tx_.clear();
      impl->queued_bytes_ = 0;
      impl->writing_ = false;
      impl->report_backpressure(impl->queued_bytes_);
      impl->state_.set_state(LinkState::Error);
      impl->notify_state();
      impl->handle_error(self, "write_queue_overflow", make_error_code(boost::system::errc::no_buffer_space));
      return;
    }
    impl->queued_bytes_ += added;
    impl->tx_.emplace_back(std::move(buf));
    impl->report_backpressure(impl->queued_bytes_);
    if (!impl->writing_) impl->do_write(self);
  });
}

void Serial::on_bytes(OnBytes cb) { impl_->on_bytes_ = std::move(cb); }
void Serial::on_state(OnState cb) { impl_->on_state_ = std::move(cb); }
void Serial::on_backpressure(OnBackpressure cb) { impl_->on_bp_ = std::move(cb); }

void Serial::set_retry_interval(unsigned interval_ms) { get_impl()->cfg_.retry_interval_ms = interval_ms; }

}  // namespace transport
}  // namespace unilink
