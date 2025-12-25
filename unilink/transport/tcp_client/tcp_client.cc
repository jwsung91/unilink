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

#include <algorithm>
#include <cstring>
#include <iostream>

#include "unilink/common/io_context_manager.hpp"
#include "unilink/common/memory_pool.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpClientConfig;
using interface::Channel;
using namespace common;  // For error_reporting namespace

std::shared_ptr<TcpClient> TcpClient::create(const TcpClientConfig& cfg) {
  return std::shared_ptr<TcpClient>(new TcpClient(cfg));
}

std::shared_ptr<TcpClient> TcpClient::create(const TcpClientConfig& cfg, net::io_context& ioc) {
  return std::shared_ptr<TcpClient>(new TcpClient(cfg, ioc));
}

TcpClient::TcpClient(const TcpClientConfig& cfg)
    : owned_ioc_(std::make_unique<net::io_context>()),
      ioc_(owned_ioc_.get()),
      strand_(net::make_strand(*ioc_)),
      resolver_(strand_),
      socket_(strand_),
      cfg_(cfg),
      retry_timer_(strand_),
      connect_timer_(strand_),
      owns_ioc_(true),
      bp_high_(cfg.backpressure_threshold) {
  // Keep the owned io_context alive even before work is posted to prevent early exit races
  work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(ioc_->get_executor());
  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  recalculate_backpressure_bounds();
  first_retry_interval_ms_ = std::min(first_retry_interval_ms_, cfg_.retry_interval_ms);
}

TcpClient::TcpClient(const TcpClientConfig& cfg, net::io_context& ioc)
    : owned_ioc_(nullptr),
      ioc_(&ioc),
      strand_(net::make_strand(*ioc_)),
      resolver_(strand_),
      socket_(strand_),
      cfg_(cfg),
      retry_timer_(strand_),
      connect_timer_(strand_),
      owns_ioc_(false),
      bp_high_(cfg.backpressure_threshold) {
  // Initialize state (ThreadSafeLinkState is already initialized in header)
  connected_ = false;
  writing_ = false;
  queue_bytes_ = 0;

  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  recalculate_backpressure_bounds();
  first_retry_interval_ms_ = std::min(first_retry_interval_ms_, cfg_.retry_interval_ms);
}

TcpClient::~TcpClient() {
  stopping_.store(true);
  // Set state to closed first
  state_.set_state(LinkState::Closed);

  // Clear callbacks to prevent dangling references
  on_bytes_ = nullptr;
  on_state_ = nullptr;
  on_bp_ = nullptr;

  // Release work guard so run() can exit cleanly
  if (work_guard_) {
    work_guard_->reset();
  }

  // Clear any pending operations
  tx_.clear();
  queue_bytes_ = 0;
  writing_ = false;

  // Clean up thread if still running and we own the io_context
  if (owns_ioc_ && ioc_ && ioc_thread_.joinable()) {
    try {
      ioc_->stop();
      ioc_thread_.join();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "destructor", "Destructor error: " + std::string(e.what()));
      error_reporting::report_system_error("tcp_client", "destructor",
                                           "Exception in destructor: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "destructor", "Unknown error in destructor");
      error_reporting::report_system_error("tcp_client", "destructor", "Unknown error in destructor");
    }
  }

  // io_context is automatically deleted by unique_ptr
}

void TcpClient::start() {
  auto current_state = state_.get_state();
  if (current_state == LinkState::Connecting || current_state == LinkState::Connected) {
    UNILINK_LOG_DEBUG("tcp_client", "start", "Start called while already active, ignoring");
    return;
  }

  if (!ioc_) {
    UNILINK_LOG_ERROR("tcp_client", "start", "io_context is null");
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  stopping_.store(false);
  retry_attempts_ = 0;
  recalculate_backpressure_bounds();

  if (ioc_->stopped()) {
    UNILINK_LOG_DEBUG("tcp_client", "start", "io_context stopped; restarting before start");
    ioc_->restart();
  }

  if (owns_ioc_) {
    // Re-arm work guard in case stop() or destructor cleared it
    work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(ioc_->get_executor());
    // Create our own thread for this io_context
    ioc_thread_ = std::thread([this]() {
      try {
        ioc_->run();
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("tcp_client", "io_context", "IO context error: " + std::string(e.what()));
        error_reporting::report_system_error("tcp_client", "io_context",
                                             "Exception in IO context: " + std::string(e.what()));
      }
    });
  }

  auto self = shared_from_this();
  net::dispatch(strand_, [self] {
    self->connected_.store(false);
    self->reset_io_objects();
    self->state_.set_state(LinkState::Connecting);
    self->notify_state();
    self->do_resolve_connect();
  });
}

void TcpClient::stop() {
  stopping_.store(true);
  auto self = shared_from_this();
  auto cleanup = [self] {
    try {
      self->state_.set_state(LinkState::Closed);
      self->retry_timer_.cancel();
      self->connect_timer_.cancel();
      self->resolver_.cancel();
      boost::system::error_code ec_cancel;
      self->socket_.cancel(ec_cancel);
      self->close_socket();
      // Clear any pending write operations
      self->tx_.clear();
      self->queue_bytes_ = 0;
      self->writing_ = false;
      self->connected_.store(false);
      self->report_backpressure(self->queue_bytes_);
      self->backpressure_active_ = false;
      if (self->owns_ioc_ && self->work_guard_) {
        self->work_guard_->reset();
      }
      self->notify_state();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "stop", "Cleanup error: " + std::string(e.what()));
      error_reporting::report_system_error("tcp_client", "stop", "Exception in stop cleanup: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "stop", "Unknown error in stop cleanup");
      error_reporting::report_system_error("tcp_client", "stop", "Unknown error in stop cleanup");
    }
  };

  if (!ioc_) {
    cleanup();
    return;
  }

  if (owns_ioc_) {
    if (!ioc_thread_.joinable()) {
      cleanup();
    } else {
      net::post(strand_, cleanup);
    }
  } else {
    if (ioc_->stopped()) {
      cleanup();
    } else {
      net::post(strand_, cleanup);
    }
  }

  if (owns_ioc_ && ioc_thread_.joinable()) {
    try {
      ioc_thread_.join();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "stop", "Stop error: " + std::string(e.what()));
      error_reporting::report_system_error("tcp_client", "stop", "Exception in stop: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "stop", "Unknown error in stop");
      error_reporting::report_system_error("tcp_client", "stop", "Unknown error in stop");
    }
  }
}

bool TcpClient::is_connected() const { return connected_.load(); }

void TcpClient::async_write_copy(const uint8_t* data, size_t size) {
  // Don't queue writes if client is stopped or in error state
  if (state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error) || !ioc_) {
    return;
  }

  if (size == 0) {
    UNILINK_LOG_WARNING("tcp_client", "async_write_copy", "Ignoring zero-length write");
    return;
  }

  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_client", "async_write_copy",
                      "Write size exceeds maximum allowed (" + std::to_string(size) + " bytes)");
    return;
  }

  // Use memory pool for better performance (only for reasonable sizes)
  if (size <= 65536) {  // Only use pool for buffers <= 64KB
    try {
      common::PooledBuffer pooled_buffer(size);
      if (pooled_buffer.valid()) {
        // Copy data to pooled buffer safely
        common::safe_memory::safe_memcpy(pooled_buffer.data(), data, size);

        const auto added = pooled_buffer.size();
        net::dispatch(strand_, [self = shared_from_this(), buf = std::move(pooled_buffer), added]() mutable {
          // Double-check state in case client was stopped while in queue
          if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
            return;
          }

          if (self->queue_bytes_ + added > self->bp_limit_) {
            UNILINK_LOG_ERROR("tcp_client", "async_write_copy",
                              "Queue limit exceeded (" + std::to_string(self->queue_bytes_ + added) + " bytes)");
            self->connected_.store(false);
            self->close_socket();
            self->tx_.clear();
            self->queue_bytes_ = 0;
            self->writing_ = false;
            self->backpressure_active_ = false;
            self->state_.set_state(LinkState::Error);
            self->notify_state();
            return;
          }

          self->queue_bytes_ += added;
          self->tx_.emplace_back(std::move(buf));
          self->report_backpressure(self->queue_bytes_);
          if (!self->writing_) self->do_write();
        });
        return;
      }
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_copy", "Failed to acquire pooled buffer: " + std::string(e.what()));
    }
  }

  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + size);
  const auto added = fallback.size();

  net::dispatch(strand_, [self = shared_from_this(), buf = std::move(fallback), added]() mutable {
    // Double-check state in case client was stopped while in queue
    if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
      return;
    }

    if (self->queue_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_copy",
                        "Queue limit exceeded (" + std::to_string(self->queue_bytes_ + added) + " bytes)");
      self->connected_.store(false);
      self->close_socket();
      self->tx_.clear();
      self->queue_bytes_ = 0;
      self->writing_ = false;
      self->backpressure_active_ = false;
      self->state_.set_state(LinkState::Error);
      self->notify_state();
      return;
    }

    self->queue_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpClient::async_write_move(std::vector<uint8_t>&& data) {
  if (state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error) || !ioc_) {
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
  net::dispatch(strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
      return;
    }

    if (self->queue_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_move",
                        "Queue limit exceeded (" + std::to_string(self->queue_bytes_ + added) + " bytes)");
      self->connected_.store(false);
      self->close_socket();
      self->tx_.clear();
      self->queue_bytes_ = 0;
      self->writing_ = false;
      self->backpressure_active_ = false;
      self->state_.set_state(LinkState::Error);
      self->notify_state();
      return;
    }

    self->queue_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpClient::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error) || !ioc_) {
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
  net::dispatch(strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
      return;
    }

    if (self->queue_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_client", "async_write_shared",
                        "Queue limit exceeded (" + std::to_string(self->queue_bytes_ + added) + " bytes)");
      self->connected_.store(false);
      self->close_socket();
      self->tx_.clear();
      self->queue_bytes_ = 0;
      self->writing_ = false;
      self->backpressure_active_ = false;
      self->state_.set_state(LinkState::Error);
      self->notify_state();
      return;
    }

    self->queue_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpClient::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void TcpClient::on_state(OnState cb) { on_state_ = std::move(cb); }
void TcpClient::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void TcpClient::do_resolve_connect() {
  auto self = shared_from_this();
  resolver_.async_resolve(cfg_.host, std::to_string(cfg_.port), [self](auto ec, tcp::resolver::results_type results) {
    if (self->stopping_.load()) {
      return;
    }
    if (ec) {
      self->schedule_retry();
      return;
    }
    // Set up connection timeout
    self->connect_timer_.expires_after(std::chrono::milliseconds(self->cfg_.connection_timeout_ms));
    self->connect_timer_.async_wait([self](const boost::system::error_code& timer_ec) {
      if (!timer_ec && !self->stopping_.load()) {
        UNILINK_LOG_ERROR("tcp_client", "connect_timeout",
                          "Connection timed out after " + std::to_string(self->cfg_.connection_timeout_ms) + "ms");
        self->handle_close(boost::asio::error::timed_out);
      }
    });

    net::async_connect(self->socket_, results, [self](auto ec2, const auto&) {
      if (self->stopping_.load()) {
        self->close_socket();
        self->connect_timer_.cancel();
        return;
      }
      if (ec2) {
        self->connect_timer_.cancel();
        self->schedule_retry();
        return;
      }
      self->connect_timer_.cancel();
      self->retry_attempts_ = 0;
      self->connected_.store(true);
      self->state_.set_state(LinkState::Connected);
      self->notify_state();
      boost::system::error_code ep_ec;
      auto rep = self->socket_.remote_endpoint(ep_ec);
      if (!ep_ec) {
        UNILINK_LOG_INFO("tcp_client", "connect",
                         "Connected to " + rep.address().to_string() + ":" + std::to_string(rep.port()));
      }
      self->start_read();
      // Flush any queued writes that accumulated while disconnected
      self->do_write();
    });
  });
}

void TcpClient::schedule_retry() {
  connected_.store(false);
  if (stopping_.load()) {
    return;
  }
  if (cfg_.max_retries != -1 && retry_attempts_ >= cfg_.max_retries) {
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }
  ++retry_attempts_;
  state_.set_state(LinkState::Connecting);
  notify_state();

  UNILINK_LOG_INFO(
      "tcp_client", "retry",
      "Scheduling retry in " +
          std::to_string((retry_attempts_ == 1 ? first_retry_interval_ms_ : cfg_.retry_interval_ms) / 1000.0) + "s");

  auto self = shared_from_this();
  const unsigned interval = retry_attempts_ == 1 ? first_retry_interval_ms_ : cfg_.retry_interval_ms;
  retry_timer_.expires_after(std::chrono::milliseconds(interval));
  retry_timer_.async_wait([self](const boost::system::error_code& ec) {
    if (!ec && !self->stopping_.load()) self->do_resolve_connect();
  });
}

void TcpClient::set_retry_interval(unsigned interval_ms) { cfg_.retry_interval_ms = interval_ms; }

void TcpClient::start_read() {
  auto self = shared_from_this();
  socket_.async_read_some(net::buffer(rx_.data(), rx_.size()), [self](auto ec, std::size_t n) {
    if (ec) {
      self->handle_close(ec);
      return;
    }
    if (self->on_bytes_) {
      try {
        self->on_bytes_(self->rx_.data(), n);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("tcp_client", "on_bytes", "Exception in on_bytes callback: " + std::string(e.what()));
        self->handle_close(make_error_code(boost::asio::error::connection_aborted));
        return;
      } catch (...) {
        UNILINK_LOG_ERROR("tcp_client", "on_bytes", "Unknown exception in on_bytes callback");
        self->handle_close(make_error_code(boost::asio::error::connection_aborted));
        return;
      }
    }
    self->start_read();
  });
}

void TcpClient::do_write() {
  // If we are not connected yet, keep the queued data and exit; writes will
  // resume once a connection is established.
  if (!connected_.load()) {
    writing_ = false;
    return;
  }

  if (tx_.empty() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) {
    writing_ = false;
    return;
  }
  writing_ = true;
  auto self = shared_from_this();

  auto current = std::move(tx_.front());
  tx_.pop_front();

  auto on_write = [self](auto ec, std::size_t n) {
    if (self->queue_bytes_ >= n) {
      self->queue_bytes_ -= n;
    } else {
      self->queue_bytes_ = 0;
    }
    self->report_backpressure(self->queue_bytes_);

    if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
      self->writing_ = false;
      return;
    }

    if (ec) {
      self->writing_ = false;
      self->handle_close(ec);
      return;
    }

    self->do_write();
  };

  // Handle PooledBuffer, shared_ptr buffer, or std::vector<uint8_t> (fallback)
  if (std::holds_alternative<common::PooledBuffer>(current)) {
    auto pooled_buf = std::get<common::PooledBuffer>(std::move(current));
    auto shared_pooled = std::make_shared<common::PooledBuffer>(std::move(pooled_buf));
    auto data = shared_pooled->data();
    auto size = shared_pooled->size();
    net::async_write(socket_, net::buffer(data, size),
                     [self, buf = shared_pooled, on_write = std::move(on_write)](auto ec, std::size_t n) mutable {
                       on_write(ec, n);
                     });
  } else if (std::holds_alternative<std::shared_ptr<const std::vector<uint8_t>>>(current)) {
    auto shared_buf = std::get<std::shared_ptr<const std::vector<uint8_t>>>(std::move(current));
    auto data = shared_buf->data();
    auto size = shared_buf->size();
    net::async_write(socket_, net::buffer(data, size),
                     [self, buf = std::move(shared_buf), on_write = std::move(on_write)](
                         auto ec, std::size_t n) mutable { on_write(ec, n); });
  } else {
    auto vec_buf = std::get<std::vector<uint8_t>>(std::move(current));
    auto shared_vec = std::make_shared<std::vector<uint8_t>>(std::move(vec_buf));
    auto data = shared_vec->data();
    auto size = shared_vec->size();
    net::async_write(
        socket_, net::buffer(data, size),
        [self, buf = shared_vec, on_write = std::move(on_write)](auto ec, std::size_t n) mutable { on_write(ec, n); });
  }
}

void TcpClient::handle_close(const boost::system::error_code& ec) {
  connected_.store(false);
  writing_ = false;
  connect_timer_.cancel();
  close_socket();
  if (stopping_.load() || state_.is_state(LinkState::Closed) || ec == boost::asio::error::operation_aborted) {
    state_.set_state(LinkState::Closed);
    notify_state();
    return;
  }
  state_.set_state(LinkState::Connecting);
  notify_state();
  schedule_retry();
}

void TcpClient::close_socket() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  socket_.close(ec);
}

void TcpClient::recalculate_backpressure_bounds() {
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

void TcpClient::report_backpressure(size_t queued_bytes) {
  if (!on_bp_) return;

  if (!backpressure_active_ && queued_bytes >= bp_high_) {
    backpressure_active_ = true;
    try {
      on_bp_(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure",
                        "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure", "Unknown exception in backpressure callback");
    }
  } else if (backpressure_active_ && queued_bytes <= bp_low_) {
    backpressure_active_ = false;
    try {
      on_bp_(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure",
                        "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "on_backpressure", "Unknown exception in backpressure callback");
    }
  }
}

void TcpClient::notify_state() {
  if (on_state_) on_state_(state_.get_state());
}

void TcpClient::reset_io_objects() {
  // All callers must already be on the strand
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
    error_reporting::report_system_error("tcp_client", "reset_io_objects",
                                         "Exception while resetting io objects: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("tcp_client", "reset_io_objects", "Unknown reset error");
    error_reporting::report_system_error("tcp_client", "reset_io_objects", "Unknown error while resetting io objects");
  }
}
}  // namespace transport
}  // namespace unilink
