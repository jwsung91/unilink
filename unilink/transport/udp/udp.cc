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
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/base/constants.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using udp = net::ip::udp;
using base::LinkState;
using concurrency::ThreadSafeLinkState;

struct UdpChannel::Impl {
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_;
  bool owns_ioc_{true};
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  udp::socket socket_;
  udp::endpoint local_endpoint_;
  udp::endpoint recv_endpoint_;
  std::optional<udp::endpoint> remote_endpoint_;

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>> tx_;
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

  explicit Impl(const config::UdpConfig& config)
      : owned_ioc_(std::make_unique<net::io_context>()),
        ioc_(owned_ioc_.get()),
        owns_ioc_(true),
        strand_(ioc_->get_executor()),
        socket_(strand_),
        cfg_(config),
        bp_high_(config.backpressure_threshold) {
    init();
  }

  Impl(const config::UdpConfig& config, net::io_context& external_ioc)
      : ioc_(&external_ioc),
        owns_ioc_(false),
        strand_(external_ioc.get_executor()),
        socket_(strand_),
        cfg_(config),
        bp_high_(config.backpressure_threshold) {
    init();
  }

  void init() {
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

  ~Impl() {
    // Note: stop() should be called from UdpChannel destructor
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

    socket_.async_receive_from(
        net::buffer(rx_), recv_endpoint_,
        [self](const boost::system::error_code& ec, std::size_t bytes) {
          self->get_impl()->handle_receive(self, ec, bytes);
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
      auto impl = self->get_impl();
      impl->queue_bytes_ = (impl->queue_bytes_ > bytes_queued) ? (impl->queue_bytes_ - bytes_queued) : 0;
      impl->report_backpressure(impl->queue_bytes_);

      if (ec == boost::asio::error::operation_aborted) {
        impl->writing_ = false;
        return;
      }

      if (impl->stop_requested_.load() || impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) ||
          impl->state_.is_state(LinkState::Error)) {
        impl->writing_ = false;
        impl->tx_.clear();
        impl->queue_bytes_ = 0;
        impl->report_backpressure(impl->queue_bytes_);
        return;
      }

      if (ec) {
        UNILINK_LOG_ERROR("udp", "write", "Send failed: " + ec.message());
        impl->transition_to(LinkState::Error, ec);
        impl->writing_ = false;
        return;
      }

      impl->writing_ = false;
      impl->do_write(self);
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
    } catch (...) {
      UNILINK_LOG_ERROR("udp", "on_state", "Unknown exception in state callback");
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
      } catch (...) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Unknown exception in backpressure callback");
      }
    } else if (backpressure_active_ && queued_bytes <= bp_low_) {
      backpressure_active_ = false;
      try {
        on_bp_(queued_bytes);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
      } catch (...) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Unknown exception in backpressure callback");
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
      on_bytes_ = nullptr;
      on_state_ = nullptr;
      on_bp_ = nullptr;
    } catch (...) {
    }
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

UdpChannel::UdpChannel(const config::UdpConfig& cfg) : impl_(std::make_unique<Impl>(cfg)) {}

UdpChannel::UdpChannel(const config::UdpConfig& cfg, net::io_context& ioc) : impl_(std::make_unique<Impl>(cfg, ioc)) {}

UdpChannel::~UdpChannel() {
  stop();
  impl_->on_bytes_ = nullptr;
  impl_->on_state_ = nullptr;
  impl_->on_bp_ = nullptr;
  impl_->join_ioc_thread(true);
}

UdpChannel::UdpChannel(UdpChannel&&) noexcept = default;
UdpChannel& UdpChannel::operator=(UdpChannel&&) noexcept = default;

void UdpChannel::start() {
  auto impl = get_impl();
  if (impl->started_) return;
  if (!impl->cfg_.is_valid()) {
    throw std::runtime_error("Invalid UDP configuration");
  }

  if (impl->owns_ioc_ && impl->owned_ioc_ && impl->owned_ioc_->stopped()) {
    impl->owned_ioc_->restart();
  }

  if (impl->ioc_thread_.joinable()) {
    impl->join_ioc_thread(false);
  }

  if (impl->owns_ioc_) {
    impl->work_guard_ =
        std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(impl->ioc_->get_executor());
  }

  auto self = shared_from_this();
  net::dispatch(impl->strand_, [self]() {
    auto impl = self->get_impl();
    impl->stop_requested_.store(false);
    impl->stopping_.store(false);
    impl->terminal_state_notified_.store(false);
    impl->connected_.store(false);
    impl->opened_.store(false);
    impl->writing_ = false;
    impl->queue_bytes_ = 0;
    impl->backpressure_active_ = false;
    impl->state_.set_state(LinkState::Idle);

    impl->transition_to(LinkState::Connecting);
    impl->open_socket(self);
  });

  if (impl->owns_ioc_) {
    impl->ioc_thread_ = std::thread([impl]() {
      try {
        impl->ioc_->run();
      } catch (...) {
      }
    });
  }

  impl->started_ = true;
}

void UdpChannel::stop() {
  auto impl = get_impl();
  if (impl->stop_requested_.exchange(true)) return;

  if (!impl->started_) {
    impl->transition_to(LinkState::Closed);
    impl->on_bytes_ = nullptr;
    impl->on_state_ = nullptr;
    impl->on_bp_ = nullptr;
    return;
  }

  impl->stopping_.store(true);
  auto self = shared_from_this();
  net::post(impl->strand_, [self]() { self->get_impl()->perform_stop_cleanup(); });

  impl->join_ioc_thread(false);

  if (impl->owns_ioc_ && impl->owned_ioc_) {
    impl->owned_ioc_->restart();
  }

  impl->started_ = false;
}

bool UdpChannel::is_connected() const { return get_impl()->connected_.load(); }

void UdpChannel::async_write_copy(memory::ConstByteSpan data) {
  auto impl = get_impl();
  if (data.empty()) return;
  if (impl->stop_requested_.load()) return;
  if (impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) || impl->state_.is_state(LinkState::Error))
    return;
  if (!impl->remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  size_t size = data.size();
  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("udp", "write", "Write size exceeds maximum allowed");
    return;
  }

  if (size > impl->bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded by single write");
    impl->transition_to(LinkState::Error);
    return;
  }

  if (impl->cfg_.enable_memory_pool && size <= 65536) {
    memory::PooledBuffer pooled(size);
    if (pooled.valid()) {
      common::safe_memory::safe_memcpy(pooled.data(), data.data(), size);
      net::post(impl->strand_, [self = shared_from_this(), buf = std::move(pooled), size]() mutable {
        auto impl = self->get_impl();
        if (!impl->enqueue_buffer(std::move(buf), size)) return;
        impl->do_write(self);
      });
      return;
    }
  }

  std::vector<uint8_t> copy(data.begin(), data.end());
  net::post(impl->strand_, [self = shared_from_this(), buf = std::move(copy), size]() mutable {
    auto impl = self->get_impl();
    if (!impl->enqueue_buffer(std::move(buf), size)) return;
    impl->do_write(self);
  });
}

void UdpChannel::async_write_move(std::vector<uint8_t>&& data) {
  auto impl = get_impl();
  auto size = data.size();
  if (size == 0) return;
  if (impl->stop_requested_.load()) return;
  if (impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) || impl->state_.is_state(LinkState::Error))
    return;
  if (!impl->remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  if (size > impl->bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded by single write");
    impl->transition_to(LinkState::Error);
    return;
  }

  net::post(impl->strand_, [self = shared_from_this(), buf = std::move(data), size]() mutable {
    auto impl = self->get_impl();
    if (!impl->enqueue_buffer(std::move(buf), size)) return;
    impl->do_write(self);
  });
}

void UdpChannel::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  auto impl = get_impl();
  if (!data || data->empty()) return;
  if (impl->stop_requested_.load()) return;
  if (impl->stopping_.load() || impl->state_.is_state(LinkState::Closed) || impl->state_.is_state(LinkState::Error))
    return;
  if (!impl->remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  auto size = data->size();
  if (size > impl->bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded by single write");
    impl->transition_to(LinkState::Error);
    return;
  }

  net::post(impl->strand_, [self = shared_from_this(), buf = std::move(data), size]() mutable {
    auto impl = self->get_impl();
    if (!impl->enqueue_buffer(std::move(buf), size)) return;
    impl->do_write(self);
  });
}

void UdpChannel::on_bytes(OnBytes cb) { impl_->on_bytes_ = std::move(cb); }

void UdpChannel::on_state(OnState cb) { impl_->on_state_ = std::move(cb); }

void UdpChannel::on_backpressure(OnBackpressure cb) { impl_->on_bp_ = std::move(cb); }

}  // namespace transport
}  // namespace unilink
