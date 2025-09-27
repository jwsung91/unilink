#include "unilink/transport/tcp_server/tcp_server_session.hpp"

#include <iostream>

namespace unilink {
namespace transport {

using namespace common;

TcpServerSession::TcpServerSession(net::io_context& ioc, tcp::socket sock)
    : ioc_(ioc), socket_(std::move(sock)) {}

void TcpServerSession::start() { start_read(); }

void TcpServerSession::async_write_copy(const uint8_t* data, size_t size) {
  if (!alive_) return; // Don't queue writes if session is not alive
  
  std::vector<uint8_t> copy(data, data + size);
  net::post(ioc_, [self = shared_from_this(), buf = std::move(copy)]() mutable {
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
  socket_.async_read_some(net::buffer(rx_.data(), rx_.size()),
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
  net::async_write(socket_, net::buffer(tx_.front()),
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

void TcpServerSession::do_close() {
  if (!alive_) return;
  alive_ = false;
  std::cout << ts_now() << "[server] client disconnected" << std::endl;
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  socket_.close(ec);
  if (on_close_) on_close_();
}

}  // namespace transport
}  // namespace unilink