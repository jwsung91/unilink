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

#include "unilink/transport/tcp_server/tcp_server_session.hpp"

#include <cstring>
#include <iostream>

#include "unilink/memory/memory_pool.hpp"
#include "unilink/transport/tcp_server/boost_tcp_socket.hpp"

namespace unilink {
namespace transport {

using namespace common;

TcpServerSession::TcpServerSession(net::io_context& ioc, tcp::socket sock, size_t backpressure_threshold)
    : ioc_(ioc),
      strand_(ioc.get_executor()),
      socket_(std::make_unique<BoostTcpSocket>(std::move(sock))),
      writing_(false),
      queue_bytes_(0),
      bp_high_(backpressure_threshold),
      alive_(false),
      cleanup_done_(false) {
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) bp_low_ = 1;
}

TcpServerSession::TcpServerSession(net::io_context& ioc, std::unique_ptr<interface::TcpSocketInterface> socket,
                                   size_t backpressure_threshold)
    : ioc_(ioc),
      strand_(ioc.get_executor()),
      socket_(std::move(socket)),
      writing_(false),
      queue_bytes_(0),
      bp_high_(backpressure_threshold),
      alive_(false),
      cleanup_done_(false) {
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) bp_low_ = 1;
}

void TcpServerSession::start() {
  if (alive_.exchange(true)) return;
  auto self = shared_from_this();
  net::dispatch(strand_, [self] { self->start_read(); });
}

void TcpServerSession::async_write_copy(const uint8_t* data, size_t size) {
  if (!alive_ || closing_) return;  // Don't queue writes if session is not alive

  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_server_session", "write", "Write size exceeds maximum allowed");
    return;
  }

  // Use memory pool for better performance (only for reasonable sizes)
  if (size <= common::constants::LARGE_BUFFER_THRESHOLD) {  // Only use pool for buffers <= 64KB
    memory::PooledBuffer pooled_buffer(size);
    if (pooled_buffer.valid()) {
      // Copy data to pooled buffer safely
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data, size);

      net::post(strand_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        if (!self->alive_ || self->closing_) return;  // Double-check in case session was closed
        if (self->queue_bytes_ + buf.size() > self->bp_limit_) {
          UNILINK_LOG_ERROR("tcp_server_session", "write", "Queue limit exceeded, closing session");
          self->do_close();
          return;
        }

        self->queue_bytes_ += buf.size();
        self->tx_.emplace_back(std::move(buf));
        self->report_backpressure(self->queue_bytes_);
        if (!self->writing_) self->do_write();
      });
      return;
    }
  }

  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + size);

  net::post(strand_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    if (!self->alive_ || self->closing_) return;  // Double-check in case session was closed
    if (self->queue_bytes_ + buf.size() > self->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_server_session", "write", "Queue limit exceeded, closing session");
      self->do_close();
      return;
    }

    self->queue_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpServerSession::async_write_move(std::vector<uint8_t>&& data) {
  if (!alive_ || closing_) return;
  const auto added = data.size();
  if (added > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_server_session", "write", "Write size exceeds maximum allowed");
    return;
  }
  net::post(strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (!self->alive_ || self->closing_) return;
    if (self->queue_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_server_session", "write", "Queue limit exceeded, closing session");
      self->do_close();
      return;
    }

    self->queue_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpServerSession::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (!alive_ || closing_ || !data) return;
  const auto added = data->size();
  if (added > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("tcp_server_session", "write", "Write size exceeds maximum allowed");
    return;
  }
  net::post(strand_, [self = shared_from_this(), buf = std::move(data), added]() mutable {
    if (!self->alive_ || self->closing_) return;
    if (self->queue_bytes_ + added > self->bp_limit_) {
      UNILINK_LOG_ERROR("tcp_server_session", "write", "Queue limit exceeded, closing session");
      self->do_close();
      return;
    }

    self->queue_bytes_ += added;
    self->tx_.emplace_back(std::move(buf));
    self->report_backpressure(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpServerSession::on_bytes(OnBytes cb) {
  auto self = shared_from_this();
  net::dispatch(strand_, [self, cb = std::move(cb)]() mutable { self->on_bytes_ = std::move(cb); });
}
void TcpServerSession::on_backpressure(OnBackpressure cb) {
  auto self = shared_from_this();
  net::dispatch(strand_, [self, cb = std::move(cb)]() mutable { self->on_bp_ = std::move(cb); });
}
void TcpServerSession::on_close(OnClose cb) {
  auto self = shared_from_this();
  net::dispatch(strand_, [self, cb = std::move(cb)]() mutable { self->on_close_ = std::move(cb); });
}

bool TcpServerSession::alive() const { return alive_.load(); }

void TcpServerSession::stop() {
  if (closing_.exchange(true)) return;
  auto self = shared_from_this();
  net::post(strand_, [self] {
    // Clear data/backpressure callbacks on the strand to block further user callbacks.
    self->on_bytes_ = nullptr;
    self->on_bp_ = nullptr;
    // Keep on_close_ so do_close can notify once.
    self->do_close();
  });
}

void TcpServerSession::start_read() {
  auto self = shared_from_this();
  socket_->async_read_some(
      net::buffer(rx_.data(), rx_.size()), net::bind_executor(strand_, [self](auto ec, std::size_t n) {
        if (self->closing_ || !self->alive_) return;
        if (ec) {
          self->do_close();
          return;
        }
        if (self->on_bytes_) {
          try {
            self->on_bytes_(self->rx_.data(), n);
          } catch (const std::exception& e) {
            UNILINK_LOG_ERROR("tcp_server_session", "on_bytes",
                              "Exception in on_bytes callback: " + std::string(e.what()));
            self->do_close();
            return;
          } catch (...) {
            UNILINK_LOG_ERROR("tcp_server_session", "on_bytes", "Unknown exception in on_bytes callback");
            self->do_close();
            return;
          }
        }
        self->start_read();
      }));
}

void TcpServerSession::do_write() {
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
    if (self->closing_ || !self->alive_) return;
    if (self->queue_bytes_ >= n) {
      self->queue_bytes_ -= n;
    } else {
      self->queue_bytes_ = 0;
    }
    self->report_backpressure(self->queue_bytes_);

    if (ec) {
      self->do_close();
      return;
    }
    self->do_write();
  };

  if (std::holds_alternative<memory::PooledBuffer>(current)) {
    auto pooled_buf = std::get<memory::PooledBuffer>(std::move(current));
    auto shared_pooled = std::make_shared<memory::PooledBuffer>(std::move(pooled_buf));
    auto data = shared_pooled->data();
    auto size = shared_pooled->size();
    socket_->async_write(net::buffer(data, size),
                         net::bind_executor(strand_, [self, buf = shared_pooled, on_write](auto ec, auto auto_n) {
                           on_write(ec, auto_n);
                         }));
  } else if (std::holds_alternative<std::shared_ptr<const std::vector<uint8_t>>>(current)) {
    auto shared_buf = std::get<std::shared_ptr<const std::vector<uint8_t>>>(std::move(current));
    auto data = shared_buf->data();
    auto size = shared_buf->size();
    socket_->async_write(net::buffer(data, size),
                         net::bind_executor(strand_, [self, buf = std::move(shared_buf), on_write](
                                                         auto ec, auto auto_n) { on_write(ec, auto_n); }));
  } else {
    auto vec_buf = std::get<std::vector<uint8_t>>(std::move(current));
    auto shared_vec = std::make_shared<std::vector<uint8_t>>(std::move(vec_buf));
    auto data = shared_vec->data();
    auto size = shared_vec->size();
    socket_->async_write(net::buffer(data, size),
                         net::bind_executor(strand_, [self, buf = shared_vec, on_write](auto ec, auto auto_n) {
                           on_write(ec, auto_n);
                         }));
  }
}

void TcpServerSession::do_close() {
  if (cleanup_done_.exchange(true)) return;  // Ensures cleanup runs only once

  alive_.store(false);
  closing_.store(true);  // Redundant, but ensures consistency

  // Safely invoke on_close callback
  auto close_cb = std::move(on_close_);

  // Clear all callbacks to prevent any further invocations
  on_bytes_ = nullptr;
  on_bp_ = nullptr;
  on_close_ = nullptr;

  UNILINK_LOG_INFO("tcp_server_session", "disconnect", "Client disconnected");
  boost::system::error_code ec;
  socket_->shutdown(tcp::socket::shutdown_both, ec);
  socket_->close(ec);

  // Release memory immediately
  tx_.clear();
  queue_bytes_ = 0;

  if (close_cb) {
    try {
      close_cb();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_close", "Exception in on_close callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_close", "Unknown exception in on_close callback");
    }
  }
}

void TcpServerSession::report_backpressure(size_t queued_bytes) {
  if (closing_ || !alive_ || !on_bp_) return;
  if (!backpressure_active_ && queued_bytes >= bp_high_) {
    backpressure_active_ = true;
    try {
      on_bp_(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure",
                        "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure", "Unknown exception in backpressure callback");
    }
  } else if (backpressure_active_ && queued_bytes <= bp_low_) {
    backpressure_active_ = false;
    try {
      on_bp_(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure",
                        "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure", "Unknown exception in backpressure callback");
    }
  }
}

}  // namespace transport
}  // namespace unilink
