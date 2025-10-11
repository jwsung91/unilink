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

#include <cstring>
#include <iostream>

#include "unilink/common/io_context_manager.hpp"
#include "unilink/common/memory_pool.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

// Specific namespace aliases instead of using namespace
using common::IoContextManager;
using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpClientConfig;
using interface::Channel;
using namespace common;  // For error_reporting namespace

TcpClient::TcpClient(const TcpClientConfig& cfg)
    : ioc_(std::make_unique<net::io_context>()),
      resolver_(*ioc_),
      socket_(*ioc_),
      cfg_(cfg),
      retry_timer_(*ioc_),
      owns_ioc_(true),
      bp_high_(cfg.backpressure_threshold) {
  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;
}

TcpClient::TcpClient(const TcpClientConfig& cfg, net::io_context& ioc)
    : ioc_(nullptr),
      resolver_(ioc),
      socket_(ioc),
      cfg_(cfg),
      retry_timer_(ioc),
      owns_ioc_(false),
      bp_high_(cfg.backpressure_threshold) {
  // Initialize state (ThreadSafeLinkState is already initialized in header)
  connected_ = false;
  writing_ = false;
  queue_bytes_ = 0;

  // Validate and clamp configuration
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;
}

TcpClient::~TcpClient() {
  // Set state to closed first
  state_.set_state(LinkState::Closed);

  // Clear callbacks to prevent dangling references
  on_bytes_ = nullptr;
  on_state_ = nullptr;
  on_bp_ = nullptr;

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
  if (owns_ioc_ && ioc_) {
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

  if (ioc_) {
    net::post(*ioc_, [this] {
      state_.set_state(LinkState::Connecting);
      notify_state();
      do_resolve_connect();
    });
  }
}

void TcpClient::stop() {
  // Set state to closed first to prevent new operations
  state_.set_state(LinkState::Closed);

  // Post cleanup work to io_context
  if (ioc_) {
    net::post(*ioc_, [this] {
      try {
        retry_timer_.cancel();
        close_socket();
        // Clear any pending write operations
        tx_.clear();
        queue_bytes_ = 0;
        writing_ = false;
      } catch (...) {
        // Ignore exceptions during cleanup
      }
    });
  }

  // Stop io_context and wait for thread to finish only if we own it
  if (owns_ioc_ && ioc_ && ioc_thread_.joinable()) {
    try {
      ioc_->stop();
      ioc_thread_.join();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_client", "stop", "Stop error: " + std::string(e.what()));
      error_reporting::report_system_error("tcp_client", "stop", "Exception in stop: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_client", "stop", "Unknown error in stop");
      error_reporting::report_system_error("tcp_client", "stop", "Unknown error in stop");
    }
  }

  try {
    notify_state();
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("tcp_client", "notify_state", "State notification error: " + std::string(e.what()));
    error_reporting::report_system_error("tcp_client", "notify_state",
                                         "Exception in state notification: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("tcp_client", "notify_state", "Unknown error in state notification");
    error_reporting::report_system_error("tcp_client", "notify_state", "Unknown error in state notification");
  }
}

bool TcpClient::is_connected() const { return connected_; }

void TcpClient::async_write_copy(const uint8_t* data, size_t size) {
  // Don't queue writes if client is stopped or in error state
  if (state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error) || !ioc_) {
    return;
  }

  // Use memory pool for better performance (only for reasonable sizes)
  if (size <= 65536) {  // Only use pool for buffers <= 64KB
    common::PooledBuffer pooled_buffer(size);
    if (pooled_buffer.valid()) {
      // Copy data to pooled buffer safely
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data, size);

      net::post(*ioc_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        // Double-check state in case client was stopped while in queue
        if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
          return;
        }

        self->queue_bytes_ += buf.size();
        self->tx_.emplace_back(std::move(buf));
        if (self->on_bp_ && self->queue_bytes_ > self->bp_high_) self->on_bp_(self->queue_bytes_);
        self->do_write();
      });
      return;
    }
  }

  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + size);

  net::post(*ioc_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    // Double-check state in case client was stopped while in queue
    if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
      return;
    }

    self->queue_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    if (self->on_bp_ && self->queue_bytes_ > self->bp_high_) self->on_bp_(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpClient::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void TcpClient::on_state(OnState cb) { on_state_ = std::move(cb); }
void TcpClient::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void TcpClient::do_resolve_connect() {
  auto self = shared_from_this();
  resolver_.async_resolve(cfg_.host, std::to_string(cfg_.port), [self](auto ec, tcp::resolver::results_type results) {
    if (ec) {
      self->schedule_retry();
      return;
    }
    net::async_connect(self->socket_, results, [self](auto ec2, const auto&) {
      if (ec2) {
        self->schedule_retry();
        return;
      }
      self->connected_ = true;
      self->state_.set_state(LinkState::Connected);
      self->notify_state();
      boost::system::error_code ep_ec;
      auto rep = self->socket_.remote_endpoint(ep_ec);
      if (!ep_ec) {
        UNILINK_LOG_INFO("tcp_client", "connect",
                         "Connected to " + rep.address().to_string() + ":" + std::to_string(rep.port()));
      }
      self->start_read();
    });
  });
}

void TcpClient::schedule_retry() {
  connected_ = false;
  state_.set_state(LinkState::Connecting);
  notify_state();

  UNILINK_LOG_INFO("tcp_client", "retry",
                   "Scheduling retry in " + std::to_string(cfg_.retry_interval_ms / 1000.0) + "s");

  auto self = shared_from_this();
  retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
  retry_timer_.async_wait([self](const boost::system::error_code& ec) {
    if (!ec) self->do_resolve_connect();
  });
}

void TcpClient::set_retry_interval(unsigned interval_ms) { cfg_.retry_interval_ms = interval_ms; }

void TcpClient::start_read() {
  auto self = shared_from_this();
  socket_.async_read_some(net::buffer(rx_.data(), rx_.size()), [self](auto ec, std::size_t n) {
    if (ec) {
      self->handle_close();
      return;
    }
    if (self->on_bytes_) self->on_bytes_(self->rx_.data(), n);
    self->start_read();
  });
}

void TcpClient::do_write() {
  if (tx_.empty() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) {
    writing_ = false;
    return;
  }
  writing_ = true;
  auto self = shared_from_this();

  // Handle both PooledBuffer and std::vector<uint8_t> (fallback)
  auto& front_buffer = tx_.front();
  if (std::holds_alternative<common::PooledBuffer>(front_buffer)) {
    auto& pooled_buf = std::get<common::PooledBuffer>(front_buffer);
    net::async_write(socket_, net::buffer(pooled_buf.data(), pooled_buf.size()), [self](auto ec, std::size_t n) {
      if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
        self->writing_ = false;
        return;
      }

      self->queue_bytes_ -= n;
      if (ec) {
        self->handle_close();
        return;
      }
      self->tx_.pop_front();
      self->do_write();
    });
  } else {
    auto& vec_buf = std::get<std::vector<uint8_t>>(front_buffer);
    net::async_write(socket_, net::buffer(vec_buf), [self](auto ec, std::size_t n) {
      if (self->state_.is_state(LinkState::Closed) || self->state_.is_state(LinkState::Error)) {
        self->writing_ = false;
        return;
      }

      self->queue_bytes_ -= n;
      if (ec) {
        self->handle_close();
        return;
      }
      self->tx_.pop_front();
      self->do_write();
    });
  }
}

void TcpClient::handle_close() {
  connected_ = false;
  close_socket();
  state_.set_state(LinkState::Connecting);
  notify_state();
  schedule_retry();
}

void TcpClient::close_socket() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  socket_.close(ec);
}

void TcpClient::notify_state() {
  if (on_state_) on_state_(state_.get_state());
}
}  // namespace transport
}  // namespace unilink