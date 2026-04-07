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

#include "unilink/transport/uds/uds_client.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "unilink/base/constants.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/error_mapping.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/transport/uds/detail/reconnect_decider.hpp"

#include "unilink/transport/uds/boost_uds_socket.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using uds = net::local::stream_protocol;

using base::LinkState;
using concurrency::ThreadSafeLinkState;
using config::UdsClientConfig;
using interface::Channel;

struct UdsClient::Impl {
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_ = nullptr;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;
  std::atomic<uint64_t> current_seq_{0};
  std::unique_ptr<interface::UdsSocketInterface> socket_;
  UdsClientConfig cfg_;
  net::steady_timer retry_timer_;
  net::steady_timer connect_timer_;
  bool owns_ioc_ = true;
  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> stopping_{false};

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<BufferVariant> tx_;
  std::optional<BufferVariant> current_write_buffer_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;
  size_t bp_low_;
  size_t bp_limit_;
  bool backpressure_active_ = false;

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

  Impl(const UdsClientConfig& cfg, net::io_context* ioc_ptr, std::unique_ptr<interface::UdsSocketInterface> socket = nullptr)
      : owned_ioc_(ioc_ptr ? nullptr : std::make_unique<net::io_context>()),
        ioc_(ioc_ptr ? ioc_ptr : owned_ioc_.get()),
        strand_(net::make_strand(*ioc_)),
        socket_(std::move(socket)),
        cfg_(cfg),
        retry_timer_(strand_),
        connect_timer_(strand_),
        owns_ioc_(!ioc_ptr),
        bp_high_(cfg.backpressure_threshold) {
    if (!socket_) {
      socket_ = std::make_unique<BoostUdsSocket>(uds::socket(strand_));
    }
    init();
  }

  void init() {
    connected_ = false;
    writing_ = false;
    queue_bytes_ = 0;
    cfg_.validate_and_clamp();
    recalculate_backpressure_bounds();
  }

  void do_connect(std::shared_ptr<UdsClient> self, uint64_t seq);
  void schedule_retry(std::shared_ptr<UdsClient> self, uint64_t seq);
  void start_read(std::shared_ptr<UdsClient> self, uint64_t seq);
  void do_write(std::shared_ptr<UdsClient> self, uint64_t seq);
  void handle_close(std::shared_ptr<UdsClient> self, uint64_t seq, const boost::system::error_code& ec = {});
  void transition_to(LinkState next, const boost::system::error_code& ec = {});
  void close_socket();
  void recalculate_backpressure_bounds();
  void report_backpressure(size_t queued_bytes);
  void record_error(diagnostics::ErrorLevel lvl, diagnostics::ErrorCategory cat, std::string_view operation,
                    const boost::system::error_code& ec, std::string_view msg, bool retryable, uint32_t retry_count);
};

std::shared_ptr<UdsClient> UdsClient::create(const UdsClientConfig& cfg) {
  return std::shared_ptr<UdsClient>(new UdsClient(cfg));
}

std::shared_ptr<UdsClient> UdsClient::create(const UdsClientConfig& cfg, boost::asio::io_context& ioc) {
  return std::shared_ptr<UdsClient>(new UdsClient(cfg, ioc));
}

std::shared_ptr<UdsClient> UdsClient::create(const UdsClientConfig& cfg,
                                             std::unique_ptr<interface::UdsSocketInterface> socket,
                                             boost::asio::io_context& ioc) {
  return std::shared_ptr<UdsClient>(new UdsClient(cfg, std::move(socket), ioc));
}

UdsClient::UdsClient(const UdsClientConfig& cfg) : impl_(std::make_unique<Impl>(cfg, nullptr)) {}
UdsClient::UdsClient(const UdsClientConfig& cfg, boost::asio::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, &ioc)) {}

UdsClient::UdsClient(const UdsClientConfig& cfg, std::unique_ptr<interface::UdsSocketInterface> socket,
                     boost::asio::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, &ioc, std::move(socket))) {}

UdsClient::~UdsClient() {
  // Use a specialized internal stop that doesn't use shared_from_this()
  impl_->stop_requested_ = true;
  uint64_t seq = impl_->current_seq_.load();

  if (impl_->ioc_) {
    net::dispatch(impl_->strand_, [this, seq]() {
      if (impl_->stopping_) return;
      impl_->stopping_ = true;
      impl_->handle_close(nullptr, seq);
    });
  }

  if (impl_->owns_ioc_ && impl_->ioc_thread_.joinable()) {
    if (std::this_thread::get_id() != impl_->ioc_thread_.get_id()) {
      impl_->ioc_->stop();
      impl_->ioc_thread_.join();
    } else {
      impl_->ioc_->stop();
      impl_->ioc_thread_.detach();
    }
  }
}

void UdsClient::start() {
  auto current_state = impl_->state_.get_state();
  if (current_state == LinkState::Connecting || current_state == LinkState::Connected) {
    return;
  }

  impl_->stop_requested_ = false;
  impl_->stopping_ = false;
  impl_->current_seq_++;
  uint64_t seq = impl_->current_seq_.load();

  if (impl_->owns_ioc_ && !impl_->ioc_thread_.joinable()) {
    impl_->work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(
        net::make_work_guard(*impl_->ioc_));
    impl_->ioc_thread_ = std::thread([this]() { impl_->ioc_->run(); });
  }

  net::post(impl_->strand_, [this, self = shared_from_this(), seq]() {
    impl_->transition_to(LinkState::Connecting);
    impl_->do_connect(self, seq);
  });
}

void UdsClient::stop() {
  bool already_stopping = impl_->stopping_.exchange(true);
  if (already_stopping) return;
  
  impl_->stop_requested_ = true;

  // Immediate cancellation of operations
  boost::system::error_code ec;
  impl_->retry_timer_.cancel(ec);
  impl_->connect_timer_.cancel(ec);
  impl_->close_socket();

  // Release work guard and STOP the io_context to break ioc->run()
  if (impl_->ioc_) {
    if (impl_->owns_ioc_) {
      impl_->work_guard_.reset();
      impl_->ioc_->stop();
    }
  }

  // Transition to Idle state (lock-free or safe call)
  impl_->state_.set_state(LinkState::Idle);
}

bool UdsClient::is_connected() const { return impl_->connected_.load(); }

void UdsClient::async_write_copy(memory::ConstByteSpan data) {
  std::vector<uint8_t> vec(data.begin(), data.end());
  async_write_move(std::move(vec));
}

void UdsClient::async_write_move(std::vector<uint8_t>&& data) {
  net::post(impl_->strand_, [this, self = shared_from_this(), data = std::move(data)]() mutable {
    size_t added = data.size();
    if (impl_->queue_bytes_ + added > impl_->bp_limit_) {
      return;
    }
    impl_->queue_bytes_ += added;
    impl_->tx_.emplace_back(std::move(data));
    impl_->report_backpressure(impl_->queue_bytes_);
    if (!impl_->writing_) {
      impl_->do_write(self, impl_->current_seq_.load());
    }
  });
}

void UdsClient::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  net::post(impl_->strand_, [this, self = shared_from_this(), data = std::move(data)]() {
    size_t added = data->size();
    if (impl_->queue_bytes_ + added > impl_->bp_limit_) {
      return;
    }
    impl_->queue_bytes_ += added;
    impl_->tx_.emplace_back(std::move(data));
    impl_->report_backpressure(impl_->queue_bytes_);
    if (!impl_->writing_) impl_->do_write(self, impl_->current_seq_.load());
  });
}

void UdsClient::on_bytes(OnBytes cb) {
  std::lock_guard<std::mutex> lock(impl_->callback_mtx_);
  impl_->on_bytes_ = std::move(cb);
}

void UdsClient::on_state(OnState cb) {
  std::lock_guard<std::mutex> lock(impl_->callback_mtx_);
  impl_->on_state_ = std::move(cb);
}

void UdsClient::on_backpressure(OnBackpressure cb) {
  std::lock_guard<std::mutex> lock(impl_->callback_mtx_);
  impl_->on_bp_ = std::move(cb);
}

void UdsClient::Impl::do_connect(std::shared_ptr<UdsClient> self, uint64_t seq) {
  uds::endpoint endpoint(cfg_.socket_path);
  
  connect_timer_.expires_after(std::chrono::milliseconds(cfg_.connection_timeout_ms));
  connect_timer_.async_wait([self, seq](const boost::system::error_code& ec) {
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) return;
    if (!ec) {
      self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "connect",
                                boost::asio::error::timed_out, "Connection timed out", true, 
                                self->impl_->reconnect_attempt_count_);
      self->impl_->handle_close(self, seq, boost::asio::error::timed_out);
    }
  });

  socket_->async_connect(endpoint, [self, seq](const boost::system::error_code& ec) {
    self->impl_->connect_timer_.cancel();
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) return;
    if (ec) {
      self->impl_->record_error(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "connect",
                                ec, "Connect failed: " + ec.message(), 
                                diagnostics::is_retryable_uds_connect_error(ec), self->impl_->reconnect_attempt_count_);
      self->impl_->schedule_retry(self, seq);
      return;
    }

    self->impl_->connected_ = true;
    self->impl_->reconnect_attempt_count_ = 0;
    self->impl_->retry_attempts_ = 0;
    self->impl_->transition_to(LinkState::Connected);
    self->impl_->start_read(self, seq);
    if (!self->impl_->tx_.empty() && !self->impl_->writing_) {
      self->impl_->do_write(self, seq);
    }
  });
}

void UdsClient::Impl::schedule_retry(std::shared_ptr<UdsClient> self, uint64_t seq) {
  transition_to(LinkState::Error);
  
  diagnostics::ErrorInfo dummy_err(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "uds_client", "connect", "Retry pending", boost::system::error_code(), true);
  auto decision = detail::decide_reconnect_uds(cfg_, dummy_err, reconnect_attempt_count_, reconnect_policy_);
  
  if (!decision.should_retry || stop_requested_.load()) {
    transition_to(LinkState::Idle);
    return;
  }

  reconnect_attempt_count_++;
  retry_timer_.expires_after(decision.delay.value_or(std::chrono::milliseconds(cfg_.retry_interval_ms)));
  retry_timer_.async_wait([self, seq](const boost::system::error_code& ec) {
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) return;
    self->impl_->do_connect(self, seq);
  });
}

void UdsClient::Impl::start_read(std::shared_ptr<UdsClient> self, uint64_t seq) {
  socket_->async_read_some(net::buffer(rx_), [self, seq](const boost::system::error_code& ec, size_t bytes) {
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) return;
    if (ec) {
      self->impl_->handle_close(self, seq, ec);
      return;
    }

    OnBytes cb;
    {
      std::lock_guard<std::mutex> lock(self->impl_->callback_mtx_);
      cb = self->impl_->on_bytes_;
    }
    if (cb) cb(memory::ConstByteSpan(self->impl_->rx_.data(), bytes));
    self->impl_->start_read(self, seq);
  });
}

void UdsClient::Impl::do_write(std::shared_ptr<UdsClient> self, uint64_t seq) {
  if (tx_.empty() || writing_) return;
  writing_ = true;
  current_write_buffer_ = std::move(tx_.front());
  tx_.pop_front();

  net::const_buffer buffer;
  std::visit([&buffer](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, std::vector<uint8_t>>) buffer = net::buffer(arg);
    else if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) buffer = net::buffer(*arg);
    else if constexpr (std::is_same_v<T, memory::PooledBuffer>) buffer = net::buffer(arg.data(), arg.size());
  }, *current_write_buffer_);

  size_t bytes_to_write = buffer.size();
  socket_->async_write(buffer, [self, seq, bytes_to_write](const boost::system::error_code& ec, size_t) {
    if (ec == net::error::operation_aborted || seq != self->impl_->current_seq_.load()) return;
    self->impl_->writing_ = false;
    self->impl_->current_write_buffer_ = std::nullopt;
    self->impl_->queue_bytes_ -= bytes_to_write;
    self->impl_->report_backpressure(self->impl_->queue_bytes_);

    if (ec) {
      self->impl_->handle_close(self, seq, ec);
      return;
    }
    if (!self->impl_->tx_.empty()) self->impl_->do_write(self, seq);
  });
}

void UdsClient::Impl::handle_close(std::shared_ptr<UdsClient> self, uint64_t seq, const boost::system::error_code&) {
  connected_ = false;
  close_socket();
  retry_timer_.cancel();
  connect_timer_.cancel();

  if (stop_requested_) {
    transition_to(LinkState::Idle);
  } else {
    schedule_retry(self, seq);
  }
}

void UdsClient::Impl::transition_to(LinkState next, const boost::system::error_code&) {
  state_.set_state(next);
  OnState cb;
  {
    std::lock_guard<std::mutex> lock(callback_mtx_);
    cb = on_state_;
  }
  if (cb) cb(next);
}

void UdsClient::Impl::close_socket() {
  boost::system::error_code ec;
  socket_->close(ec);
}

void UdsClient::Impl::recalculate_backpressure_bounds() {
  bp_limit_ = cfg_.backpressure_threshold * 2;
}

void UdsClient::Impl::report_backpressure(size_t queued_bytes) {
  OnBackpressure cb;
  {
    std::lock_guard<std::mutex> lock(callback_mtx_);
    cb = on_bp_;
  }
  if (cb) cb(queued_bytes);
}

void UdsClient::Impl::record_error(diagnostics::ErrorLevel lvl, diagnostics::ErrorCategory cat, std::string_view operation,
                                  const boost::system::error_code& ec, std::string_view msg, bool retryable, uint32_t) {
  std::lock_guard<std::mutex> lock(last_err_mtx_);
  last_error_info_ = diagnostics::ErrorInfo(lvl, cat, "uds_client", operation, msg, ec, retryable);
}

}  // namespace transport
}  // namespace unilink
