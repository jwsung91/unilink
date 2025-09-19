#include "module/tcp_client.hpp"

#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

TcpClient::TcpClient(net::io_context& ioc, const TcpClientConfig& cfg)
    : ioc_(ioc), resolver_(ioc), socket_(ioc), cfg_(cfg), retry_timer_(ioc) {}

void TcpClient::start() {
  state_ = LinkState::Connecting;
  notify_state();
  do_resolve_connect();
}

void TcpClient::stop() {
  retry_timer_.cancel();
  close_socket();
  state_ = LinkState::Closed;
  notify_state();
}

bool TcpClient::is_connected() const { return connected_; }

void TcpClient::async_write_copy(const uint8_t* data, size_t size) {
  std::vector<uint8_t> copy(data, data + size);
  net::post(ioc_, [self = shared_from_this(), buf = std::move(copy)]() mutable {
    self->queue_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    if (self->on_bp_ && self->queue_bytes_ > self->bp_high_)
      self->on_bp_(self->queue_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void TcpClient::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void TcpClient::on_state(OnState cb) { on_state_ = std::move(cb); }
void TcpClient::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void TcpClient::do_resolve_connect() {
  auto self = shared_from_this();
  resolver_.async_resolve(
      cfg_.host, std::to_string(cfg_.port),
      [self](auto ec, tcp::resolver::results_type results) {
        if (ec) {
          self->schedule_retry();
          return;
        }
        net::async_connect(
            self->socket_, results, [self](auto ec2, const auto&) {
              if (ec2) {
                self->schedule_retry();
                return;
              }
              self->connected_ = true;
              self->state_ = LinkState::Connected;
              self->notify_state();
              boost::system::error_code ep_ec;
              auto rep = self->socket_.remote_endpoint(ep_ec);
              if (!ep_ec) {
                std::cout << ts_now() << " [client] connected to "
                          << rep.address().to_string() << ":" << rep.port()
                          << std::endl;
              }
              self->start_read();
            });
      });
}

void TcpClient::schedule_retry() {
  connected_ = false;
  state_ = LinkState::Connecting;
  notify_state();

  std::cout << ts_now() << " [client] retry in "
            << (cfg_.retry_interval_ms / 1000.0) << "s (fixed)" << std::endl;

  auto self = shared_from_this();
  retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
  retry_timer_.async_wait([self](const boost::system::error_code& ec) {
    if (!ec) self->do_resolve_connect();
  });
}

void TcpClient::start_read() {
  auto self = shared_from_this();
  socket_.async_read_some(net::buffer(rx_.data(), rx_.size()),
                          [self](auto ec, std::size_t n) {
                            if (ec) {
                              self->handle_close();
                              return;
                            }
                            if (self->on_bytes_)
                              self->on_bytes_(self->rx_.data(), n);
                            self->start_read();
                          });
}

void TcpClient::do_write() {
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
                       self->handle_close();
                       return;
                     }
                     self->tx_.pop_front();
                     self->do_write();
                   });
}

void TcpClient::handle_close() {
  connected_ = false;
  close_socket();
  state_ = LinkState::Connecting;
  notify_state();
  schedule_retry();
}

void TcpClient::close_socket() {
  boost::system::error_code ec;
  socket_.shutdown(tcp::socket::shutdown_both, ec);
  socket_.close(ec);
}

void TcpClient::notify_state() {
  if (on_state_) on_state_(state_);
}
