#include "unilink/transport/tcp_server/tcp_server_session.hpp"

#include <iostream>
#include <cstring>

#include "unilink/transport/tcp_server/boost_tcp_socket.hpp"
#include "unilink/common/memory_pool.hpp"

namespace unilink {
namespace transport {

using namespace common;

TcpServerSession::TcpServerSession(net::io_context& ioc, tcp::socket sock, size_t backpressure_threshold)
    : ioc_(ioc), socket_(std::make_unique<BoostTcpSocket>(std::move(sock))), 
      writing_(false), queue_bytes_(0), bp_high_(backpressure_threshold), alive_(false) {}

TcpServerSession::TcpServerSession(net::io_context& ioc, std::unique_ptr<interface::TcpSocketInterface> socket, size_t backpressure_threshold)
    : ioc_(ioc), socket_(std::move(socket)), 
      writing_(false), queue_bytes_(0), bp_high_(backpressure_threshold), alive_(false) {}

void TcpServerSession::start() { start_read(); }

void TcpServerSession::async_write_copy(const uint8_t* data, size_t size) {
  if (!alive_) return; // Don't queue writes if session is not alive
  
  // Use memory pool for better performance (only for reasonable sizes)
  if (size <= 65536) { // Only use pool for buffers <= 64KB
    common::PooledBuffer pooled_buffer(size);
    if (pooled_buffer.valid()) {
      // Copy data to pooled buffer safely
      common::safe_memory::safe_memcpy(pooled_buffer.data(), data, size);
      
      net::post(ioc_, [self = shared_from_this(), buf = std::move(pooled_buffer)]() mutable {
        if (!self->alive_) return; // Double-check in case session was closed
        self->queue_bytes_ += buf.size();
        self->tx_.emplace_back(std::move(buf));
        if (self->on_bp_ && self->queue_bytes_ > self->bp_high_)
          self->on_bp_(self->queue_bytes_);
        if (!self->writing_) self->do_write();
      });
      return;
    }
  }
  
  // Fallback to regular allocation for large buffers or pool exhaustion
  std::vector<uint8_t> fallback(data, data + size);
  
  net::post(ioc_, [self = shared_from_this(), buf = std::move(fallback)]() mutable {
    if (!self->alive_) return; // Double-check in case session was closed
    self->queue_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    if (self->on_bp_ && self->queue_bytes_ > self->bp_high_)
      self->on_bp_(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpServerSession::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void TcpServerSession::on_backpressure(OnBackpressure cb) {
  on_bp_ = std::move(cb);
}
void TcpServerSession::on_close(OnClose cb) { on_close_ = std::move(cb); }
bool TcpServerSession::alive() const { return alive_; }

void TcpServerSession::start_read() {
  alive_ = true;
  auto self = shared_from_this();
  socket_->async_read_some(net::buffer(rx_.data(), rx_.size()),
                          [self](auto ec, std::size_t n) {
                            if (ec) {
                              self->do_close();
                              return;
                            }
                            if (self->on_bytes_)
                              self->on_bytes_(self->rx_.data(), n);
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
    socket_->async_write(net::buffer(pooled_buf.data(), pooled_buf.size()),
                         [self](auto ec, std::size_t n) {
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
    socket_->async_write(net::buffer(vec_buf),
                         [self](auto ec, std::size_t n) {
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
  if (!alive_) return;
  alive_ = false;
  UNILINK_LOG_INFO("tcp_server_session", "disconnect", "Client disconnected");
  boost::system::error_code ec;
  socket_->shutdown(tcp::socket::shutdown_both, ec);
  socket_->close(ec);
  if (on_close_) on_close_();
}

}  // namespace transport
}  // namespace unilink