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

#include "unilink/transport/tcp_client/tcp_client.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "unilink/base/constants.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/transport/tcp_client/detail/reconnect_logic.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

using base::LinkState;
using concurrency::ThreadSafeLinkState;
using config::TcpClientConfig;
using interface::Channel;
using namespace common;  // For error_reporting namespace

struct TcpClient::Impl {
  // Members moved from TcpClient
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_ = nullptr;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;
  std::atomic<uint64_t> lifecycle_seq_{0};
  std::atomic<uint64_t> stop_seq_{0};
  std::atomic<uint64_t> current_seq_{0};
  tcp::resolver resolver_;
  tcp::socket socket_;
  TcpClientConfig cfg_;
  net::steady_timer retry_timer_;
  net::steady_timer connect_timer_;
  bool owns_ioc_ = true;
  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> stopping_{false};
  std::atomic<bool> terminal_state_notified_{false};
  std::atomic<bool> reconnect_pending_{false};

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<BufferVariant> tx_;
  std::optional<BufferVariant> current_write_buffer_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;
  size_t bp_low_;
  size_t bp_limit_;
  bool backpressure_active_ = false;
  unsigned first_retry_interval_ms_ = 100;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  mutable std::mutex callback_mtx_;
  std::atomic<bool> connected_{false};
  ThreadSafeLinkState state_{LinkState::Idle};
  int retry_attempts_ = 0;
  uint32_t reconnect_attempt_count_{0};
  std::optional<ReconnectPolicy> reconnect_policy_;

  mutable std::mutex last_err_mtx_;
  std::optional<diagnostics::ErrorInfo> last_error_info_;

  Impl(const TcpClientConfig& cfg, net::io_context* ioc_ptr)
      : owned_ioc_(ioc_ptr ? nullptr : std::make_unique<net::io_context>()),
        ioc_(ioc_ptr ? ioc_ptr : owned_ioc_.get()),
        strand_(net::make_strand(*ioc_)),
        resolver_(strand_),
        socket_(strand_),
        cfg_(cfg),
        retry_timer_(strand_),
        connect_timer_(strand_),
        owns_ioc_(!ioc_ptr),
        bp_high_(cfg.backpressure_threshold) {
    init();
  }

  void init() {
    connected_ = false;
    writing_ = false;
    queue_bytes_ = 0;
    cfg_.validate_and_clamp();
    recalculate_backpressure_bounds();
    first_retry_interval_ms_ = std::min(first_retry_interval_ms_, cfg_.retry_interval_ms);
  }

  void do_resolve_connect(std::shared_ptr<TcpClient> self, uint64_t seq);
  void schedule_retry(std::shared_ptr<TcpClient> self, uint64_t seq);
  void start_read(std::shared_ptr<TcpClient> self, uint64_t seq);
  void do_write(std::shared_ptr<TcpClient> self, uint64_t seq);
  void handle_close(std::shared_ptr<TcpClient> self, uint64_t seq, const boost::system::error_code& ec = {});
  void transition_to(LinkState next, const boost::system::error_code& ec = {});
  void perform_stop_cleanup();
  void reset_start_state();
  void join_ioc_thread(bool allow_detach);
  void close_socket();
  void recalculate_backpressure_bounds();
  void report_backpressure(size_t queued_bytes);
  void notify_state();
  void reset_io_objects();
  void record_error(diagnostics::ErrorLevel lvl, diagnostics::ErrorCategory cat, std::string_view operation,
                    const boost::system::error_code& ec, std::string_view msg, bool retryable, uint32_t retry_count);
};

std::shared_ptr<TcpClient> TcpClient::create(const TcpClientConfig& cfg) {
  return std::shared_ptr<TcpClient>(new TcpClient(cfg));
}

std::shared_ptr<TcpClient> TcpClient::create(const TcpClientConfig& cfg, boost::asio::io_context& ioc) {
  return std::shared_ptr<TcpClient>(new TcpClient(cfg, ioc));
}

TcpClient::TcpClient(const TcpClientConfig& cfg) : impl_(std::make_unique<Impl>(cfg, nullptr)) {}
TcpClient::TcpClient(const TcpClientConfig& cfg, boost::asio::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, &ioc)) {}

TcpClient::~TcpClient() {
  stop();
  impl_->join_ioc_thread(true);

  impl_->on_bytes_ = nullptr;
  impl_->on_state_ = nullptr;
  impl_->on_bp_ = nullptr;
}

TcpClient::TcpClient(TcpClient&&) noexcept = default;
TcpClient& TcpClient::operator=(TcpClient&&) noexcept = default;

std::optional<diagnostics::ErrorInfo> TcpClient::last_error_info() const {
  std::lock_guard<std::mutex> lock(impl_->last_err_mtx_);
  return impl_->last_error_info_;
}

void TcpClient::start() {
  auto current_state = impl_->state_.get_state();
  if (current_state == LinkState::Connecting || current_state == LinkState::Connected) {
    UNILINK_LOG_DEBUG("tcp_client", "start", "Start called while already active, ignoring");
    return;
  }

  if (!impl_->ioc_) {
    UNILINK_LOG_ERROR("tcp_client", "start", "io_context is null");
  }

  impl_->recalculate_backpressure_bounds();

  if (impl_->ioc_ && impl_->ioc_->stopped()) {
    UNILINK_LOG_DEBUG("tcp_client", "start", "io_context stopped; restarting before start");
    impl_->ioc_->restart();
  }

  if (impl_->ioc_thread_.joinable()) {
    impl_->join_ioc_thread(false);
  }

  const auto seq = impl_->lifecycle_seq_.fetch_add(1) + 1;
  impl_->current_seq_.store(seq);

  if (impl_->owns_ioc_ && impl_->ioc_) {
    impl_->work_guard_ =
        std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(impl_->ioc_->get_executor());
    impl_->ioc_thread_ = std::thread([this]() {
      try {
        impl_->ioc_->run();
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("tcp_client", "io_context", "IO context error: " + std::string(e.what()));
        diagnostics::error_reporting::report_system_error("tcp_client", "io_context",
                                                          "Exception in IO context: " + std::string(e.what()));
      }
    });
  }

  auto weak_self = weak_from_this();
  if (impl_->ioc_) {
    net::dispatch(impl_->strand_, [weak_self, seq] {
      if (auto self = weak_self.lock()) {
        if (seq <= self->impl_->stop_seq_.load()) {
          return;
        }
        self->impl_->reset_start_state();
        self->impl_->connected_.store(false);
        self->impl_->reset_io_objects();
        self->impl_->transition_to(LinkState::Connecting);
        self->impl_->do_resolve_connect(self, seq);
      }
    });
  } else {
    UNILINK_LOG_ERROR("tcp_client", "start", "io_context is null");
  }
}

void TcpClient::stop() {
  if (impl_->stop_requested_.exchange(true)) {
    return;
  }

  impl_->stopping_.store(true);
  impl_->stop_seq_.store(impl_->current_seq_.load());
  auto weak_self = weak_from_this();
  if (!impl_->ioc_) {
    return;
  }

  if (auto self = weak_self.lock()) {
    net::post(impl_->strand_, [self]() { self->impl_->perform_stop_cleanup(); });
  }

  impl_->join_ioc_thread(false);
}

bool TcpClient::is_connected() const { return get_impl()->connected_.load(); }

void TcpClient::async_write_copy(memory::ConstByteSpan data) {
  if (impl_->stop_requested_.load() || impl_->state_.is_state(LinkState::Closed) ||
      impl_->state_.is_state(LinkState::Error) || !impl_->ioc_) {
    return;
  }

  size_t size = data.size();
  if (size == 0) {
    UNILINK_LOG_WARNING("tcp_client", "async_write_copy", "Ignoring zero-length write");
    return;
  }

  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_client", "async_write_copy",
                      "Write size exceeds maximum allowed (" + std::to_string(size) + " bytes)");
    return;
  }

  if (size <= 65536) {
    try {
      memory::PooledBuffer pooled_buffer(size);
      if (pooled_buffer.valid()) {
        common::safe_memory::safe_memcpy(pooled_buffer.data(), data.data(), size);
        const auto added = pooled_buffer.size();
        net::dispatch(impl_->strand_, [self = shared_from_this(), buf = std::move(pooled_buffer), added]() mutable {
          if (self->impl_->stop_requested_.load() || self->impl_->state_.is_state(LinkState::Closed) ||
              self->impl_->state_.is_state(LinkState::Error)) {
            return;
          }

          if (self->impl_->queue_bytes_ + added > self->impl_->bp_limit_) {
            UNILINK_LOG_ERROR("tcp_client", "async_write_copy",
                              "Queue limit exceeded (" + std::to_string(self->impl_->queue_bytes_ + added) + " bytes)");
            self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::COMMUNICATION,
                                      "async_write_copy", boost::asio::error::no_buffer_space, "Queue limit exceeded",
                                      false, 0);
            self->impl_->connected_.store(false);
            self->impl_->close_socket();
            self->impl_->tx_.clear();
            self->impl_->queue_bytes_ = 0;
            self->impl_->writing_ = false;
            self->impl_->backpressure_active_ = false;
            self->impl_->transition_to(LinkState::Error);
            return;
          }

          self->impl_->queue_bytes_ += added;
          self->impl_->tx_.emplace_back(std::move(buf));
          self->impl_->report_backpressure(self->impl_->queue_bytes_);
          if (!self->impl_->writing_) self->impl_->do_write(self, self->impl_->current_seq_.load());
        });
        return;
      }
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_copy", "Failed to acquire pooled buffer: " + std::string(e.what()));
    }
  }

  std::vector<uint8_t> fallback(data.begin(), data.end());
  const auto added = fallback.size();

  net::dispatch(impl_->strand_, [self = shared_from_this(), buf = std::move(fallback), added]() mutable {
    if (self->impl_->stop_requested_.load() || self->impl_->state_.is_state(LinkState::Closed) ||
        self->impl_->state_.is_state(LinkState::Error)) {
      return;
    }

    if (self->impl_->queue_bytes_ + added > self->impl_->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_copy",
                        "Queue limit exceeded (" + std::to_string(self->impl_->queue_bytes_ + added) + " bytes)");
      self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::COMMUNICATION,
                                "async_write_copy", boost::asio::error::no_buffer_space, "Queue limit exceeded", false,
                                0);
      self->impl_->connected_.store(false);
      self->impl_->close_socket();
      self->impl_->tx_.clear();
      self->impl_->queue_bytes_ = 0;
      self->impl_->writing_ = false;
      self->impl_->backpressure_active_ = false;
      self->impl_->transition_to(LinkState::Error);
      return;
    }

    self->impl_->queue_bytes_ += added;
    self->impl_->tx_.emplace_back(std::move(buf));
    self->impl_->report_backpressure(self->impl_->queue_bytes_);
    if (!self->impl_->writing_) self->impl_->do_write(self, self->impl_->current_seq_.load());
  });
}

void TcpClient::async_write_move(std::vector<uint8_t>&& data) {
  if (impl_->stop_requested_.load() || impl_->state_.is_state(LinkState::Closed) ||
      impl_->state_.is_state(LinkState::Error) || !impl_->ioc_) {
    return;
  }
  const auto size = data.size();
  if (size == 0) {
    UNILINK_LOG_WARNING("tcp_client", "async_write_move", "Ignoring zero-length write");
    return;
  }
  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_client", "async_write_move",
                      "Write size exceeds maximum allowed (" + std::to_string(size) + " bytes)");
    return;
  }

  const auto added = size;
  net::dispatch(impl_->strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->impl_->stop_requested_.load() || self->impl_->state_.is_state(LinkState::Closed) ||
        self->impl_->state_.is_state(LinkState::Error)) {
      return;
    }

    if (self->impl_->queue_bytes_ + added > self->impl_->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_move",
                        "Queue limit exceeded (" + std::to_string(self->impl_->queue_bytes_ + added) + " bytes)");
      self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::COMMUNICATION,
                                "async_write_move", boost::asio::error::no_buffer_space, "Queue limit exceeded", false,
                                0);
      self->impl_->connected_.store(false);
      self->impl_->close_socket();
      self->impl_->tx_.clear();
      self->impl_->queue_bytes_ = 0;
      self->impl_->writing_ = false;
      self->impl_->backpressure_active_ = false;
      self->impl_->transition_to(LinkState::Error);
      return;
    }

    self->impl_->queue_bytes_ += added;
    self->impl_->tx_.emplace_back(std::move(buf));
    self->impl_->report_backpressure(self->impl_->queue_bytes_);
    if (!self->impl_->writing_) self->impl_->do_write(self, self->impl_->current_seq_.load());
  });
}

void TcpClient::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (impl_->stop_requested_.load() || impl_->state_.is_state(LinkState::Closed) ||
      impl_->state_.is_state(LinkState::Error) || !impl_->ioc_) {
    return;
  }
  if (!data || data->empty()) {
    UNILINK_LOG_WARNING("tcp_client", "async_write_shared", "Ignoring empty shared buffer");
    return;
  }
  const auto size = data->size();
  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_client", "async_write_shared",
                      "Write size exceeds maximum allowed (" + std::to_string(size) + " bytes)");
    return;
  }

  const auto added = size;
  net::dispatch(impl_->strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->impl_->stop_requested_.load() || self->impl_->state_.is_state(LinkState::Closed) ||
        self->impl_->state_.is_state(LinkState::Error)) {
      return;
    }

    if (self->impl_->queue_bytes_ + added > self->impl_->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_shared",
                        "Queue limit exceeded (" + std::to_string(self->impl_->queue_bytes_ + added) + " bytes)");
      self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::COMMUNICATION,
                                "async_write_shared", boost::asio::error::no_buffer_space, "Queue limit exceeded",
                                false, 0);
      self->impl_->connected_.store(false);
      self->impl_->close_socket();
      self->impl_->tx_.clear();
      self->impl_->queue_bytes_ = 0;
      self->impl_->writing_ = false;
      self->impl_->backpressure_active_ = false;
      self->impl_->transition_to(LinkState::Error);
      return;
    }

    self->impl_->queue_bytes_ += added;
    self->impl_->tx_.emplace_back(std::move(buf));
    self->impl_->report_backpressure(self->impl_->queue_bytes_);
    if (!self->impl_->writing_) self->impl_->do_write(self, self->impl_->current_seq_.load());
  });
}

void TcpClient::on_bytes(OnBytes cb) {
  std::lock_guard<std::mutex> lock(impl_->callback_mtx_);
  impl_->on_bytes_ = std::move(cb);
}
void TcpClient::on_state(OnState cb) {
  std::lock_guard<std::mutex> lock(impl_->callback_mtx_);
  impl_->on_state_ = std::move(cb);
}
void TcpClient::on_backpressure(OnBackpressure cb) {
  std::lock_guard<std::mutex> lock(impl_->callback_mtx_);
  impl_->on_bp_ = std::move(cb);
}
void TcpClient::set_retry_interval(unsigned interval_ms) { impl_->cfg_.retry_interval_ms = interval_ms; }
void TcpClient::set_reconnect_policy(ReconnectPolicy policy) {
  if (policy) {
    impl_->reconnect_policy_ = std::move(policy);
  } else {
    impl_->reconnect_policy_ = std::nullopt;
  }
}

// Impl methods implementation

void TcpClient::Impl::do_resolve_connect(std::shared_ptr<TcpClient> self, uint64_t seq) {
  resolver_.async_resolve(
      cfg_.host, std::to_string(cfg_.port), [self, seq](auto ec, tcp::resolver::results_type results) {
        if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) {
          return;
        }
        if (self->impl_->stop_requested_.load() || self->impl_->stopping_.load()) {
          return;
        }
        if (ec) {
          uint32_t current_attempts = self->impl_->reconnect_policy_
                                          ? self->impl_->reconnect_attempt_count_
                                          : static_cast<uint32_t>(self->impl_->retry_attempts_);
          self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "resolve",
                                    ec, "Resolution failed: " + ec.message(), true, current_attempts);
          self->impl_->schedule_retry(self, seq);
          return;
        }
        self->impl_->connect_timer_.expires_after(std::chrono::milliseconds(self->impl_->cfg_.connection_timeout_ms));
        self->impl_->connect_timer_.async_wait([self, seq](const boost::system::error_code& timer_ec) {
          if (timer_ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) {
            return;
          }
          if (!timer_ec && !self->impl_->stop_requested_.load() && !self->impl_->stopping_.load()) {
            UNILINK_LOG_ERROR(
                "tcp_client", "connect_timeout",
                "Connection timed out after " + std::to_string(self->impl_->cfg_.connection_timeout_ms) + "ms");
            uint32_t current_attempts = self->impl_->reconnect_policy_
                                            ? self->impl_->reconnect_attempt_count_
                                            : static_cast<uint32_t>(self->impl_->retry_attempts_);
            self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "connect",
                                      boost::asio::error::timed_out, "Connection timed out", true, current_attempts);
            self->impl_->handle_close(self, seq, boost::asio::error::timed_out);
          }
        });

        net::async_connect(self->impl_->socket_, results, [self, seq](auto ec2, const auto&) {
          if (ec2 == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) {
            return;
          }
          if (self->impl_->stop_requested_.load() || self->impl_->stopping_.load()) {
            self->impl_->close_socket();
            self->impl_->connect_timer_.cancel();
            return;
          }
          if (ec2) {
            self->impl_->connect_timer_.cancel();
            uint32_t current_attempts = self->impl_->reconnect_policy_
                                            ? self->impl_->reconnect_attempt_count_
                                            : static_cast<uint32_t>(self->impl_->retry_attempts_);
            self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "connect",
                                      ec2, "Connection failed: " + ec2.message(), true, current_attempts);
            self->impl_->schedule_retry(self, seq);
            return;
          }
          self->impl_->connect_timer_.cancel();
          self->impl_->retry_attempts_ = 0;
          self->impl_->reconnect_attempt_count_ = 0;
          self->impl_->connected_.store(true);

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
          int yes = 1;
          (void)::setsockopt(self->impl_->socket_.native_handle(), SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#endif

          self->impl_->transition_to(LinkState::Connected);
          boost::system::error_code ep_ec;
          auto rep = self->impl_->socket_.remote_endpoint(ep_ec);
          if (!ep_ec) {
            UNILINK_LOG_INFO("tcp_client", "connect",
                             "Connected to " + rep.address().to_string() + ":" + std::to_string(rep.port()));
          }
          self->impl_->start_read(self, seq);
          self->impl_->do_write(self, seq);
        });
      });
}

void TcpClient::Impl::schedule_retry(std::shared_ptr<TcpClient> self, uint64_t seq) {
  connected_.store(false);
  if (stop_requested_.load() || stopping_.load()) {
    return;
  }

  // Prevent double scheduling of reconnect
  if (reconnect_pending_.exchange(true)) {
    return;
  }

  std::optional<diagnostics::ErrorInfo> last_err;
  {
    std::lock_guard<std::mutex> lock(last_err_mtx_);
    last_err = last_error_info_;
  }

  if (!last_err) {
    last_err = diagnostics::ErrorInfo(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION,
                                      "tcp_client", "schedule_retry", "Unknown error",
                                      make_error_code(boost::asio::error::not_connected), true);
  }

  // Determine current attempt count based on active mode
  uint32_t current_attempts = reconnect_policy_ ? reconnect_attempt_count_ : static_cast<uint32_t>(retry_attempts_);

  auto decision = detail::decide_reconnect(cfg_, *last_err, current_attempts, reconnect_policy_);

  if (!decision.should_retry) {
    UNILINK_LOG_INFO("tcp_client", "retry", "Reconnect stopped by policy/config");
    transition_to(LinkState::Error);
    reconnect_pending_.store(false);
    return;
  }

  std::chrono::milliseconds delay;
  if (decision.delay) {
    // Policy-based delay
    delay = *decision.delay;
    reconnect_attempt_count_++;
  } else {
    // Legacy delay logic
    ++retry_attempts_;
    delay = std::chrono::milliseconds(retry_attempts_ == 1 ? first_retry_interval_ms_ : cfg_.retry_interval_ms);
  }

  transition_to(LinkState::Connecting);

  UNILINK_LOG_INFO("tcp_client", "retry",
                   "Scheduling retry in " + std::to_string(static_cast<double>(delay.count()) / 1000.0) + "s");

  retry_timer_.expires_after(delay);
  retry_timer_.async_wait([self, seq](const boost::system::error_code& ec) {
    // Clear pending flag regardless of result (fired or aborted)
    self->impl_->reconnect_pending_.store(false);

    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) {
      return;
    }
    if (!ec && !self->impl_->stop_requested_.load() && !self->impl_->stopping_.load())
      self->impl_->do_resolve_connect(self, seq);
  });
}

void TcpClient::Impl::start_read(std::shared_ptr<TcpClient> self, uint64_t seq) {
  socket_.async_read_some(net::buffer(rx_.data(), rx_.size()), [self, seq](auto ec, std::size_t n) {
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) {
      return;
    }
    if (self->impl_->stop_requested_.load()) {
      return;
    }
    if (ec) {
      self->impl_->handle_close(self, seq, ec);
      return;
    }
    OnBytes on_bytes;
    {
      std::lock_guard<std::mutex> lock(self->impl_->callback_mtx_);
      on_bytes = self->impl_->on_bytes_;
    }

    if (on_bytes) {
      try {
        on_bytes(memory::ConstByteSpan(self->impl_->rx_.data(), n));
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("tcp_client", "on_bytes", "Exception in on_bytes callback: " + std::string(e.what()));
        self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::COMMUNICATION, "on_bytes",
                                  boost::asio::error::connection_aborted,
                                  "Exception in on_bytes: " + std::string(e.what()), false, 0);
        self->impl_->handle_close(self, seq, make_error_code(boost::asio::error::connection_aborted));
        return;
      } catch (...) {
        UNILINK_LOG_ERROR("tcp_client", "on_bytes", "Unknown exception in on_bytes callback");
        self->impl_->handle_close(self, seq, make_error_code(boost::asio::error::connection_aborted));
        return;
      }
    }
    self->impl_->start_read(self, seq);
  });
}

void TcpClient::Impl::do_write(std::shared_ptr<TcpClient> self, uint64_t seq) {
  if (stop_requested_.load()) {
    tx_.clear();
    queue_bytes_ = 0;
    writing_ = false;
    report_backpressure(queue_bytes_);
    return;
  }

  if (!connected_.load()) {
    writing_ = false;
    return;
  }

  if (tx_.empty() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) {
    writing_ = false;
    return;
  }
  writing_ = true;

  current_write_buffer_ = std::move(tx_.front());
  tx_.pop_front();

  auto& current = *current_write_buffer_;

  auto queued_bytes = std::visit(
      [](auto&& buf) -> size_t {
        using Buffer = std::decay_t<decltype(buf)>;
        if constexpr (std::is_same_v<Buffer, std::shared_ptr<const std::vector<uint8_t>>>) {
          return buf ? buf->size() : 0;
        } else {
          return buf.size();
        }
      },
      current);

  auto on_write = [self, queued_bytes, seq](auto ec, std::size_t) {
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) {
      self->impl_->current_write_buffer_.reset();
      self->impl_->queue_bytes_ =
          (self->impl_->queue_bytes_ > queued_bytes) ? (self->impl_->queue_bytes_ - queued_bytes) : 0;
      self->impl_->report_backpressure(self->impl_->queue_bytes_);
      self->impl_->writing_ = false;
      return;
    }

    if (ec) {
      if (self->impl_->current_write_buffer_) {
        self->impl_->tx_.push_front(std::move(*self->impl_->current_write_buffer_));
      }
      self->impl_->current_write_buffer_.reset();

      UNILINK_LOG_ERROR("tcp_client", "do_write", "Write failed: " + ec.message());
      self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::COMMUNICATION, "write", ec,
                                "Write failed: " + ec.message(), false, 0);
      self->impl_->writing_ = false;
      self->impl_->handle_close(self, seq, ec);
      return;
    }

    self->impl_->current_write_buffer_.reset();
    self->impl_->queue_bytes_ =
        (self->impl_->queue_bytes_ > queued_bytes) ? (self->impl_->queue_bytes_ - queued_bytes) : 0;
    self->impl_->report_backpressure(self->impl_->queue_bytes_);

    if (self->impl_->stop_requested_.load() || self->impl_->state_.is_state(LinkState::Closed) ||
        self->impl_->state_.is_state(LinkState::Error)) {
      self->impl_->writing_ = false;
      return;
    }

    self->impl_->do_write(self, seq);
  };

  std::visit(
      [&](const auto& buf) {
        using T = std::decay_t<decltype(buf)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
          net::async_write(socket_, net::buffer(buf->data(), buf->size()), on_write);
        } else {
          net::async_write(socket_, net::buffer(buf.data(), buf.size()), on_write);
        }
      },
      current);
}

void TcpClient::Impl::handle_close(std::shared_ptr<TcpClient> self, uint64_t seq, const boost::system::error_code& ec) {
  if (ec == net::error::operation_aborted || seq != current_seq_.load()) {
    return;
  }
  UNILINK_LOG_INFO("tcp_client", "handle_close", "Closing connection. Error: " + ec.message());
  if (ec) {
    bool retry = (cfg_.max_retries == -1 || retry_attempts_ < cfg_.max_retries);
    uint32_t current_attempts = reconnect_policy_ ? reconnect_attempt_count_ : static_cast<uint32_t>(retry_attempts_);

    if (reconnect_policy_) {
      retry = true;
    }

    record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "handle_close", ec,
                 "Connection closed with error: " + ec.message(), retry, current_attempts);
  }
  connected_.store(false);
  writing_ = false;
  connect_timer_.cancel();
  close_socket();
  if (stop_requested_.load() || stopping_.load() || state_.is_state(LinkState::Closed)) {
    transition_to(LinkState::Closed, ec);
    return;
  }
  transition_to(LinkState::Connecting, ec);
  schedule_retry(self, seq);
}

void TcpClient::Impl::close_socket() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  socket_.close(ec);
}

void TcpClient::Impl::recalculate_backpressure_bounds() {
  bp_high_ = cfg_.backpressure_threshold;
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) {
    bp_low_ = 1;
  }
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  if (bp_limit_ < bp_high_) {
    bp_limit_ = bp_high_;
  }
  backpressure_active_ = false;
}

void TcpClient::Impl::report_backpressure(size_t queued_bytes) {
  if (stop_requested_.load() || stopping_.load()) return;

  OnBackpressure on_bp;
  {
    std::lock_guard<std::mutex> lock(callback_mtx_);
    on_bp = on_bp_;
  }
  if (!on_bp) return;

  if (!backpressure_active_ && queued_bytes >= bp_high_) {
    backpressure_active_ = true;
    try {
      on_bp(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure",
                        "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure", "Unknown exception in backpressure callback");
    }
  } else if (backpressure_active_ && queued_bytes <= bp_low_) {
    backpressure_active_ = false;
    try {
      on_bp(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure",
                        "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure", "Unknown exception in backpressure callback");
    }
  }
}

void TcpClient::Impl::transition_to(LinkState next, const boost::system::error_code& ec) {
  if (ec == net::error::operation_aborted) {
    return;
  }

  const auto current = state_.get_state();
  const bool retrying_same_state = (next == LinkState::Connecting && current == LinkState::Connecting);
  if ((current == LinkState::Closed || current == LinkState::Error) &&
      (next == LinkState::Closed || next == LinkState::Error)) {
    return;
  }

  if (next == LinkState::Closed || next == LinkState::Error) {
    if (terminal_state_notified_.exchange(true)) {
      return;
    }
  } else if (current == next && !retrying_same_state) {
    return;
  }

  state_.set_state(next);
  notify_state();
}

void TcpClient::Impl::perform_stop_cleanup() {
  try {
    retry_timer_.cancel();
    connect_timer_.cancel();
    resolver_.cancel();
    boost::system::error_code ec_cancel;
    socket_.cancel(ec_cancel);
    close_socket();
    tx_.clear();
    queue_bytes_ = 0;
    writing_ = false;
    connected_.store(false);
    backpressure_active_ = false;

    if (owns_ioc_ && work_guard_) {
      work_guard_->reset();
    }
    transition_to(LinkState::Closed);
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("tcp_client", "stop_cleanup", "Cleanup error: " + std::string(e.what()));
    record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::SYSTEM, "stop_cleanup", {},
                 "Cleanup error: " + std::string(e.what()), false, 0);
    diagnostics::error_reporting::report_system_error("tcp_client", "stop_cleanup",
                                                      "Exception in stop cleanup: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("tcp_client", "stop_cleanup", "Unknown error in stop cleanup");
    diagnostics::error_reporting::report_system_error("tcp_client", "stop_cleanup", "Unknown error in stop cleanup");
  }
}

void TcpClient::Impl::reset_start_state() {
  stop_requested_.store(false);
  stopping_.store(false);
  terminal_state_notified_.store(false);
  reconnect_pending_.store(false);
  retry_attempts_ = 0;
  reconnect_attempt_count_ = 0;
  connected_.store(false);
  writing_ = false;
  queue_bytes_ = 0;
  backpressure_active_ = false;
  state_.set_state(LinkState::Idle);
}

void TcpClient::Impl::join_ioc_thread(bool allow_detach) {
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
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("tcp_client", "join", "Join failed: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("tcp_client", "join", "Join failed with unknown error");
  }
}

void TcpClient::Impl::notify_state() {
  if (stop_requested_.load() || stopping_.load()) return;

  OnState on_state;
  {
    std::lock_guard<std::mutex> lock(callback_mtx_);
    on_state = on_state_;
  }
  if (!on_state) return;

  try {
    on_state(state_.get_state());
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("tcp_client", "on_state", "Exception in state callback: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("tcp_client", "on_state", "Unknown exception in state callback");
  }
}

void TcpClient::Impl::record_error(diagnostics::ErrorLevel lvl, diagnostics::ErrorCategory cat,
                                   std::string_view operation, const boost::system::error_code& ec,
                                   std::string_view msg, bool retryable, uint32_t retry_count) {
  std::lock_guard<std::mutex> lock(last_err_mtx_);
  diagnostics::ErrorInfo info(lvl, cat, "tcp_client", operation, msg, ec, retryable);
  info.retry_count = retry_count;
  last_error_info_ = info;
}

void TcpClient::Impl::reset_io_objects() {
  try {
    boost::system::error_code ec_cancel;
    socket_.cancel(ec_cancel);
    close_socket();
    socket_ = tcp::socket(strand_);
    resolver_.cancel();
    resolver_ = tcp::resolver(strand_);
    retry_timer_ = net::steady_timer(strand_);
    connect_timer_ = net::steady_timer(strand_);
    tx_.clear();
    queue_bytes_ = 0;
    writing_ = false;
    backpressure_active_ = false;
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("tcp_client", "reset_io_objects", "Reset error: " + std::string(e.what()));
    record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::SYSTEM, "reset_io_objects", {},
                 "Reset error: " + std::string(e.what()), false, 0);
    diagnostics::error_reporting::report_system_error("tcp_client", "reset_io_objects",
                                                      "Exception while resetting io objects: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("tcp_client", "reset_io_objects", "Unknown reset error");
    diagnostics::error_reporting::report_system_error("tcp_client", "reset_io_objects",
                                                      "Unknown error while resetting io objects");
  }
}

}  // namespace transport
}  // namespace unilink
