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
  net::dispatch(strand_, [self, cb = std::move(cb)]() mutable {
    if (self->closing_.load() || self->cleanup_done_.load()) return;
    self->on_bytes_ = std::move(cb);
  });
}
void TcpServerSession::on_backpressure(OnBackpressure cb) {
  auto self = shared_from_this();
  net::dispatch(strand_, [self, cb = std::move(cb)]() mutable {
    if (self->closing_.load() || self->cleanup_done_.load()) return;
    self->on_bp_ = std::move(cb);
  });
}
void TcpServerSession::on_close(OnClose cb) {
  auto self = shared_from_this();
  net::dispatch(strand_, [self, cb = std::move(cb)]() mutable {
    if (self->closing_.load() || self->cleanup_done_.load()) return;
    self->on_close_ = std::move(cb);
  });
}

bool TcpServerSession::alive() const { return alive_.load(); }

void TcpServerSession::stop() {
  if (closing_.exchange(true)) return;
  auto self = shared_from_this();
  net::post(strand_, [self] {
    // Clear callbacks on the strand to block further user callbacks after stop.
    self->on_bytes_ = nullptr;
    self->on_bp_ = nullptr;
    self->on_close_ = nullptr;
    self->do_close();
  });
}

void TcpServerSession::cancel() {
  auto self = shared_from_this();
  net::dispatch(strand_, [self] {
    boost::system::error_code ec;
    // Cancelling the socket via close() causes ongoing operations to complete with operation_aborted.
    // Unlike stop(), this does NOT set closing_ flag immediately, allowing the
    // error handler to run normally and trigger do_close() via the error path.
    if (self->socket_) {
      self->socket_->close(ec);
    }
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
  // Optimization: Move into current_write_buffer_ to keep it alive during async op
  // without allocating a shared_ptr control block.
  current_write_buffer_ = std::move(tx_.front());
  tx_.pop_front();

  auto& current = *current_write_buffer_;

  auto on_write = [self](const boost::system::error_code& ec, std::size_t n) {
    // Release the buffer immediately
    self->current_write_buffer_.reset();

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

  std::visit(
      [&](const auto& buf) {
        using T = std::decay_t<decltype(buf)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
          socket_->async_write(net::buffer(buf->data(), buf->size()), net::bind_executor(strand_, on_write));
        } else {
          socket_->async_write(net::buffer(buf.data(), buf.size()), net::bind_executor(strand_, on_write));
        }
      },
      current);
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
