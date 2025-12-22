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

#include "unilink/common/memory_pool.hpp"
#include "unilink/transport/tcp_server/boost_tcp_socket.hpp"

namespace unilink {
namespace transport {

using namespace common;

TcpServerSession::TcpServerSession(net::io_context& ioc, tcp::socket sock, size_t backpressure_threshold)
    : ioc_(ioc),
      socket_(std::make_unique<BoostTcpSocket>(std::move(sock))),
      writing_(false),
      queue_bytes_(0),
      bp_high_(backpressure_threshold),
      alive_(false) {}

TcpServerSession::TcpServerSession(net::io_context& ioc, std::unique_ptr<interface::TcpSocketInterface> socket,
                                   size_t backpressure_threshold)
    : ioc_(ioc),
      socket_(std::move(socket)),
      writing_(false),
      queue_bytes_(0),
      bp_high_(backpressure_threshold),
      alive_(false) {}

void TcpServerSession::start() { start_read(); }

void TcpServerSession::async_write_copy(const uint8_t* data, size_t size) {
  if (!alive_) return;  // Don't queue writes if session is not alive

  // Use memory pool for better performance (only for reasonable sizes)
  if (size <= common::constants::LARGE_BUFFER_THRESHOLD) {  // Only use pool for buffers <= 64KB
    common::PooledBuffer pooled_buffer(size);
    if (pooled_buffer.valid()) {
      // Copy data to pooled buffer safely
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data, size);

      net::post(ioc_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        if (!self->alive_) return;  // Double-check in case session was closed
        self->queue_bytes_ += buf.size();
        self->tx_.emplace_back(std::move(buf));
        if (self->on_bp_ && self->queue_bytes_ > self->bp_high_) {
          try {
            self->on_bp_(self->queue_bytes_);
          } catch (const std::exception& e) {
            UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure",
                              "Exception in backpressure callback: " + std::string(e.what()));
          } catch (...) {
            UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure",
                              "Unknown exception in backpressure callback");
          }
        }
        if (!self->writing_) self->do_write();
      });
      return;
    }
  }

  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + size);

  net::post(ioc_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    if (!self->alive_) return;  // Double-check in case session was closed
    self->queue_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    if (self->on_bp_ && self->queue_bytes_ > self->bp_high_) {
      try {
        self->on_bp_(self->queue_bytes_);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure",
                          "Exception in backpressure callback: " + std::string(e.what()));
      } catch (...) {
        UNILINK_LOG_ERROR("tcp_server_session", "on_backpressure",
                          "Unknown exception in backpressure callback");
      }
    }
    if (!self->writing_) self->do_write();
  });
}

void TcpServerSession::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void TcpServerSession::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }
void TcpServerSession::on_close(OnClose cb) { on_close_ = std::move(cb); }
bool TcpServerSession::alive() const { return alive_.load(); }

void TcpServerSession::stop() {
  auto self = shared_from_this();
  net::post(ioc_, [self] { self->do_close(); });
}

void TcpServerSession::start_read() {
  alive_.store(true);
  auto self = shared_from_this();
  socket_->async_read_some(net::buffer(rx_.data(), rx_.size()), [self](auto ec, std::size_t n) {
    if (ec) {
      self->do_close();
      return;
    }
    if (self->on_bytes_) {
      try {
        self->on_bytes_(self->rx_.data(), n);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("tcp_server_session", "on_bytes", "Exception in on_bytes callback: " + std::string(e.what()));
        self->do_close();
        return;
      } catch (...) {
        UNILINK_LOG_ERROR("tcp_server_session", "on_bytes", "Unknown exception in on_bytes callback");
        self->do_close();
        return;
      }
    }
    self->start_read();
  });
}

void TcpServerSession::do_write() {
  if (tx_.empty()) {
    writing_ = false;
    return;
  }
  writing_ = true;
  auto self = shared_from_this();

  // Handle both PooledBuffer and std::vector<uint8_t> (fallback)
  auto& front_buffer = tx_.front();
  if (std::holds_alternative<common::PooledBuffer>(front_buffer)) {
    auto& pooled_buf = std::get<common::PooledBuffer>(front_buffer);
    socket_->async_write(net::buffer(pooled_buf.data(), pooled_buf.size()), [self](auto ec, std::size_t n) {
      self->queue_bytes_ -= n;
      if (ec) {
        self->do_close();
        return;
      }
      self->tx_.pop_front();
      self->do_write();
    });
  } else {
    auto& vec_buf = std::get<std::vector<uint8_t>>(front_buffer);
    socket_->async_write(net::buffer(vec_buf), [self](auto ec, std::size_t n) {
      self->queue_bytes_ -= n;
      if (ec) {
        self->do_close();
        return;
      }
      self->tx_.pop_front();
      self->do_write();
    });
  }
}

void TcpServerSession::do_close() {
  if (!alive_.exchange(false)) return;
  UNILINK_LOG_INFO("tcp_server_session", "disconnect", "Client disconnected");
  boost::system::error_code ec;
  socket_->shutdown(tcp::socket::shutdown_both, ec);
  socket_->close(ec);
  if (on_close_) {
    try {
      on_close_();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_close", "Exception in on_close callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_server_session", "on_close", "Unknown exception in on_close callback");
    }
  }
}

}  // namespace transport
}  // namespace unilink
