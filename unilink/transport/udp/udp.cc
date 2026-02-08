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

#include "unilink/transport/udp/udp.hpp"

#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/base/constants.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using udp = net::ip::udp;
using base::LinkState;
using concurrency::ThreadSafeLinkState;

struct UdpChannel::Impl {
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_;
  bool owns_ioc_;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  udp::socket socket_;
  udp::endpoint local_endpoint_;
  udp::endpoint recv_endpoint_;
  std::optional<udp::endpoint> remote_endpoint_;

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};

  using BufferVariant =
      std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>;
  std::deque<BufferVariant> tx_;

  bool writing_{false};
  size_t queue_bytes_{0};
  config::UdpConfig cfg_;
  size_t bp_high_;
  size_t bp_low_;
  size_t bp_limit_;
  bool backpressure_active_{false};

  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> stopping_{false};
  std::atomic<bool> opened_{false};
  std::atomic<bool> connected_{false};
  bool started_{false};
  ThreadSafeLinkState state_{LinkState::Idle};
  std::atomic<bool> terminal_state_notified_{false};

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  Impl(const config::UdpConfig& cfg, net::io_context* ioc_ptr)
      : owned_ioc_(ioc_ptr ? nullptr : std::make_unique<net::io_context>()),
        ioc_(ioc_ptr ? ioc_ptr : owned_ioc_.get()),
        owns_ioc_(!ioc_ptr),
        strand_(ioc_->get_executor()),
        socket_(strand_),
        cfg_(cfg),
        bp_high_(cfg.backpressure_threshold) {
    cfg_.validate_and_clamp();
    bp_high_ = cfg_.backpressure_threshold;
    bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
    if (bp_low_ == 0) bp_low_ = 1;
    bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                         common::constants::MAX_BUFFER_SIZE);
    if (bp_limit_ < bp_high_) {
      bp_limit_ = bp_high_;
    }
    set_remote_from_config();
  }

  void start(std::shared_ptr<UdpChannel> self) {
    if (started_) return;
    if (!cfg_.is_valid()) {
      throw std::runtime_error("Invalid UDP configuration");
    }

    if (owns_ioc_ && owned_ioc_ && owned_ioc_->stopped()) {
      owned_ioc_->restart();
    }

    if (ioc_thread_.joinable()) {
      join_ioc_thread(false);
    }

    if (owns_ioc_) {
      work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(ioc_->get_executor());
    }

    net::dispatch(strand_, [self]() {
      self->impl_->reset_start_state();
      self->impl_->transition_to(LinkState::Connecting);
      self->impl_->open_socket(self);
    });

    if (owns_ioc_) {
      ioc_thread_ = std::thread([this]() {
        try {
          ioc_->run();
        } catch (const std::exception& e) {
          UNILINK_LOG_ERROR("udp", "run", "Unhandled exception: " + std::string(e.what()));
        } catch (...) {
          UNILINK_LOG_ERROR("udp", "run", "Unknown unhandled exception");
        }
      });
    }

    started_ = true;
  }

  void stop(std::shared_ptr<UdpChannel> self) {
    if (stop_requested_.exchange(true)) return;

    if (!started_) {
      transition_to(LinkState::Closed);
      clear_callbacks();
      return;
    }

    stopping_.store(true);

    // Use weak_ptr to prevent keeping self alive indefinitely if already destroying
    std::weak_ptr<UdpChannel> weak_self = self;

    auto cleanup_task = [weak_self]() {
      if (auto s = weak_self.lock()) {
        s->impl_->perform_stop_cleanup();
      }
    };

    if (!ioc_) {
      perform_stop_cleanup();
    } else if (self) {
       net::post(strand_, cleanup_task);
    } else {
       perform_stop_cleanup();
    }

    join_ioc_thread(false);

    if (owns_ioc_ && owned_ioc_) {
      owned_ioc_->restart();
    }

    started_ = false;
  }

  void open_socket(std::shared_ptr<UdpChannel> self) {
    if (stopping_.load() || stop_requested_.load()) return;

    boost::system::error_code ec;
    auto address = net::ip::make_address(cfg_.local_address, ec);
    if (ec) {
      UNILINK_LOG_ERROR("udp", "bind", "Invalid local address: " + cfg_.local_address);
      transition_to(LinkState::Error, ec);
      return;
    }

    local_endpoint_ = udp::endpoint(address, cfg_.local_port);
    socket_.open(local_endpoint_.protocol(), ec);
    if (ec) {
      UNILINK_LOG_ERROR("udp", "open", "Socket open failed: " + ec.message());
      transition_to(LinkState::Error, ec);
      return;
    }

    socket_.bind(local_endpoint_, ec);
    if (ec) {
      UNILINK_LOG_ERROR("udp", "bind", "Bind failed: " + ec.message());
      transition_to(LinkState::Error, ec);
      return;
    }

    opened_.store(true);
    if (remote_endpoint_) {
      connected_.store(true);
      transition_to(LinkState::Connected);
    } else {
      transition_to(LinkState::Listening);
    }
    start_receive(self);
  }

  void start_receive(std::shared_ptr<UdpChannel> self) {
    if (stopping_.load() || stop_requested_.load() || state_.is_state(LinkState::Closed) ||
        state_.is_state(LinkState::Error) || !socket_.is_open()) {
      return;
    }

    socket_.async_receive_from(net::buffer(rx_), recv_endpoint_,
                               [self](const boost::system::error_code& ec, std::size_t bytes) {
                                 self->impl_->handle_receive(self, ec, bytes);
                               });
  }

  void handle_receive(std::shared_ptr<UdpChannel> self, const boost::system::error_code& ec, std::size_t bytes) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }

    if (stopping_.load() || stop_requested_.load() || state_.is_state(LinkState::Closed) ||
        state_.is_state(LinkState::Error)) {
      return;
    }

    if (ec == boost::asio::error::message_size || bytes >= rx_.size()) {
      UNILINK_LOG_ERROR("udp", "receive", "Datagram truncated (buffer too small)");
      transition_to(LinkState::Error, ec);
      return;
    }

    if (ec) {
      UNILINK_LOG_ERROR("udp", "receive", "Receive failed: " + ec.message());
      transition_to(LinkState::Error, ec);
      return;
    }

    if (!remote_endpoint_) {
      remote_endpoint_ = recv_endpoint_;
      connected_.store(true);
      transition_to(LinkState::Connected);
    }

    if (bytes > 0 && on_bytes_) {
      try {
        on_bytes_(memory::ConstByteSpan(rx_.data(), bytes));
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("udp", "on_bytes", "Exception in bytes callback: " + std::string(e.what()));
        if (cfg_.stop_on_callback_exception) {
          transition_to(LinkState::Error);
          return;
        }
      } catch (...) {
        UNILINK_LOG_ERROR("udp", "on_bytes", "Unknown exception in bytes callback");
        if (cfg_.stop_on_callback_exception) {
          transition_to(LinkState::Error);
          return;
        }
      }
    }

    start_receive(self);
  }

  void do_write(std::shared_ptr<UdpChannel> self) {
    if (writing_ || tx_.empty()) return;
    if (stop_requested_.load() || stopping_.load() || state_.is_state(LinkState::Closed) ||
        state_.is_state(LinkState::Error)) {
      tx_.clear();
      queue_bytes_ = 0;
      writing_ = false;
      backpressure_active_ = false;
      report_backpressure(queue_bytes_);
      return;
    }
    if (!remote_endpoint_) {
      UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
      writing_ = false;
      return;
    }

    writing_ = true;

    auto current = std::move(tx_.front());
    tx_.pop_front();
    auto bytes_queued = std::visit(
        [](auto&& buf) -> size_t {
          using Buffer = std::decay_t<decltype(buf)>;
          if constexpr (std::is_same_v<Buffer, std::shared_ptr<const std::vector<uint8_t>>>) {
            return buf ? buf->size() : 0;
          } else {
            return buf.size();
          }
        },
        current);

    auto on_write = [self, bytes_queued](const boost::system::error_code& ec, std::size_t) {
      self->impl_->queue_bytes_ =
          (self->impl_->queue_bytes_ > bytes_queued) ? (self->impl_->queue_bytes_ - bytes_queued) : 0;
      self->impl_->report_backpressure(self->impl_->queue_bytes_);

      if (ec == boost::asio::error::operation_aborted) {
        self->impl_->writing_ = false;
        return;
      }

      if (self->impl_->stop_requested_.load() || self->impl_->stopping_.load() ||
          self->impl_->state_.is_state(LinkState::Closed) || self->impl_->state_.is_state(LinkState::Error)) {
        self->impl_->writing_ = false;
        self->impl_->tx_.clear();
        self->impl_->queue_bytes_ = 0;
        self->impl_->report_backpressure(self->impl_->queue_bytes_);
        return;
      }

      if (ec) {
        UNILINK_LOG_ERROR("udp", "write", "Send failed: " + ec.message());
        self->impl_->transition_to(LinkState::Error, ec);
        self->impl_->writing_ = false;
        return;
      }

      self->impl_->writing_ = false;
      self->impl_->do_write(self);
    };

    std::visit(
        [&](auto&& buf) {
          using T = std::decay_t<decltype(buf)>;

          auto* data_ptr = [&]() {
            if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
              return buf->data();
            } else {
              return buf.data();
            }
          }();

          auto size = [&]() {
            if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
              return buf->size();
            } else {
              return buf.size();
            }
          }();

          socket_.async_send_to(
              net::buffer(data_ptr, size), *remote_endpoint_,
              [buf_captured = std::move(buf), on_write = std::move(on_write)](
                  const boost::system::error_code& ec, std::size_t bytes) mutable { on_write(ec, bytes); });
        },
        std::move(current));
  }

  void close_socket() {
    boost::system::error_code ec;
    socket_.cancel(ec);
    socket_.close(ec);
  }

  void notify_state() {
    if (!on_state_) return;
    try {
      on_state_(state_.get_state());
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("udp", "on_state", "Exception in state callback: " + std::string(e.what()));
    }
  }

  void report_backpressure(size_t queued_bytes) {
    if (stop_requested_.load() || !on_bp_) return;

    if (!backpressure_active_ && queued_bytes >= bp_high_) {
      backpressure_active_ = true;
      try {
        on_bp_(queued_bytes);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
      }
    } else if (backpressure_active_ && queued_bytes <= bp_low_) {
      backpressure_active_ = false;
      try {
        on_bp_(queued_bytes);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
      }
    }
  }

  bool enqueue_buffer(
      std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>&& buffer,
      size_t size) {
    if (stopping_.load() || stop_requested_.load() || state_.is_state(LinkState::Closed) ||
        state_.is_state(LinkState::Error)) {
      return false;
    }

    if (queue_bytes_ + size > bp_limit_) {
      UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded");
      transition_to(LinkState::Error);
      return false;
    }
    queue_bytes_ += size;
    tx_.push_back(std::move(buffer));
    report_backpressure(queue_bytes_);
    return true;
  }

  void set_remote_from_config() {
    if (!cfg_.remote_address || !cfg_.remote_port) return;
    boost::system::error_code ec;
    auto addr = net::ip::make_address(*cfg_.remote_address, ec);
    if (ec) {
      throw std::runtime_error("Invalid remote address: " + *cfg_.remote_address);
    }
    remote_endpoint_ = udp::endpoint(addr, *cfg_.remote_port);
  }

  void transition_to(LinkState target, const boost::system::error_code& ec = {}) {
    if (ec == net::error::operation_aborted) {
      return;
    }

    const auto current = state_.get_state();
    if ((current == LinkState::Closed || current == LinkState::Error) &&
        (target == LinkState::Closed || target == LinkState::Error)) {
      return;
    }

    if (target == LinkState::Closed || target == LinkState::Error) {
      if (terminal_state_notified_.exchange(true)) {
        return;
      }
    } else if (current == target) {
      return;
    }

    state_.set_state(target);
    notify_state();
  }

  void clear_callbacks() {
    on_bytes_ = nullptr;
    on_state_ = nullptr;
    on_bp_ = nullptr;
  }

  void perform_stop_cleanup() {
    try {
      close_socket();
      tx_.clear();
      queue_bytes_ = 0;
      writing_ = false;
      const bool had_backpressure = backpressure_active_;
      backpressure_active_ = false;
      if (had_backpressure && on_bp_) {
        try {
          on_bp_(queue_bytes_);
        } catch (...) {
        }
      }
      connected_.store(false);
      opened_.store(false);
      if (owns_ioc_ && ioc_) {
        ioc_->stop();
      }
      if (owns_ioc_ && work_guard_) {
        work_guard_->reset();
      }
      transition_to(LinkState::Closed);
      clear_callbacks();
    } catch (...) {
      UNILINK_LOG_ERROR("udp", "stop_cleanup", "Unknown error in stop cleanup");
    }
  }

  void reset_start_state() {
    stop_requested_.store(false);
    stopping_.store(false);
    terminal_state_notified_.store(false);
    connected_.store(false);
    opened_.store(false);
    writing_ = false;
    queue_bytes_ = 0;
    backpressure_active_ = false;
    state_.set_state(LinkState::Idle);
  }

  void join_ioc_thread(bool allow_detach) {
    if (!owns_ioc_ || !ioc_thread_.joinable()) {
      return;
    }

    if (std::this_thread::get_id() == ioc_thread_.get_id()) {
      if (allow_detach) {
        ioc_thread_.detach();
      }
      return;
    }

    try {
      ioc_thread_.join();
    } catch (...) {
    }
  }
};

std::shared_ptr<UdpChannel> UdpChannel::create(const config::UdpConfig& cfg) {
  return std::shared_ptr<UdpChannel>(new UdpChannel(cfg));
}

std::shared_ptr<UdpChannel> UdpChannel::create(const config::UdpConfig& cfg, net::io_context& ioc) {
  return std::shared_ptr<UdpChannel>(new UdpChannel(cfg, ioc));
}

UdpChannel::UdpChannel(const config::UdpConfig& cfg) : impl_(std::make_unique<Impl>(cfg, nullptr)) {}

UdpChannel::UdpChannel(const config::UdpConfig& cfg, net::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, &ioc)) {}

UdpChannel::~UdpChannel() {
  stop();
}

void UdpChannel::start() { impl_->start(shared_from_this()); }

void UdpChannel::stop() {
  std::shared_ptr<UdpChannel> self;
  try {
    self = shared_from_this();
  } catch (const std::bad_weak_ptr&) {
  }
  impl_->stop(self);
}

bool UdpChannel::is_connected() const { return impl_->connected_.load(); }

void UdpChannel::async_write_copy(memory::ConstByteSpan data) {
  if (data.empty()) return;
  if (impl_->stop_requested_.load()) return;

  size_t size = data.size();
  if (size > common::constants::MAX_BUFFER_SIZE) return;

  if (impl_->cfg_.enable_memory_pool && size <= 65536) {
    memory::PooledBuffer pooled(size);
    if (pooled.valid()) {
      common::safe_memory::safe_memcpy(pooled.data(), data.data(), size);
      net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(pooled), size]() mutable {
        if (self->impl_->enqueue_buffer(std::move(buf), size)) {
           self->impl_->do_write(self);
        }
      });
      return;
    }
  }

  std::vector<uint8_t> copy(data.begin(), data.end());
  net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(copy), size]() mutable {
    if (self->impl_->enqueue_buffer(std::move(buf), size)) {
       self->impl_->do_write(self);
    }
  });
}

void UdpChannel::async_write_move(std::vector<uint8_t>&& data) {
  size_t size = data.size();
  if (size == 0) return;
  net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(data), size]() mutable {
    if (self->impl_->enqueue_buffer(std::move(buf), size)) {
       self->impl_->do_write(self);
    }
  });
}

void UdpChannel::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (!data || data->empty()) return;
  size_t size = data->size();
  net::post(impl_->strand_, [self = shared_from_this(), buf = std::move(data), size]() mutable {
    if (self->impl_->enqueue_buffer(std::move(buf), size)) {
       self->impl_->do_write(self);
    }
  });
}

void UdpChannel::on_bytes(OnBytes cb) { impl_->on_bytes_ = std::move(cb); }
void UdpChannel::on_state(OnState cb) { impl_->on_state_ = std::move(cb); }
void UdpChannel::on_backpressure(OnBackpressure cb) { impl_->on_bp_ = std::move(cb); }

}  // namespace transport
}  // namespace unilink
