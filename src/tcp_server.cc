 #include "tcp_server.hpp"
 #include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

TcpServerSession::TcpServerSession(net::io_context& ioc, tcp::socket sock)
    : ioc_(ioc), socket_(std::move(sock)) {}

void TcpServerSession::start() { start_read(); }

void TcpServerSession::async_write_copy(const uint8_t* data, size_t size) {
  std::vector<uint8_t> copy(data, data + size);
  net::post(ioc_,
            [self = shared_from_this(), buf = std::move(copy)]() mutable {
              self->queue_bytes_ += buf.size();
              self->tx_.emplace_back(std::move(buf));
              if (self->on_bp_ && self->queue_bytes_ > self->bp_high_)
                self->on_bp_(self->queue_bytes_);
              if (!self->writing_) self->do_write();
            });
}

void TcpServerSession::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void TcpServerSession::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }
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

TcpServer::TcpServer(net::io_context& ioc, uint16_t port)
    : ioc_(ioc), acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {}

void TcpServer::start() {
  state_ = LinkState::Listening;
  notify_state();
  do_accept();
}

void TcpServer::stop() {
  boost::system::error_code ec;
  acceptor_.close(ec);
  if (sess_) sess_.reset();
  state_ = LinkState::Closed;
  notify_state();
}

bool TcpServer::is_connected() const { return sess_ && sess_->alive(); }

void TcpServer::async_write_copy(const uint8_t* data, size_t size) {
  if (sess_) sess_->async_write_copy(data, size);
}

void TcpServer::on_bytes(OnBytes cb) {
  on_bytes_ = std::move(cb);
  if (sess_) sess_->on_bytes(on_bytes_);
}
void TcpServer::on_state(OnState cb) { on_state_ = std::move(cb); }
void TcpServer::on_backpressure(OnBackpressure cb) {
  on_bp_ = std::move(cb);
  if (sess_) sess_->on_backpressure(on_bp_);
}

void TcpServer::do_accept() {
  auto self = shared_from_this();
  acceptor_.async_accept([self](auto ec, tcp::socket sock) {
    if (ec) {
      std::cout << ts_now() << "[server] accept error: " << ec.message()
                << std::endl;
      self->state_ = LinkState::Error;
      self->notify_state();
      self->do_accept();
      return;
    }
    boost::system::error_code ep_ec;
    auto rep = sock.remote_endpoint(ep_ec);
    if (!ep_ec) {
      std::cout << ts_now() << "[server] accepted "
                << rep.address().to_string() << ":" << rep.port()
                << std::endl;
    } else {
      std::cout << ts_now() << "[server] accepted (endpoint unknown)"
                << std::endl;
    }

    self->sess_ =
        std::make_shared<TcpServerSession>(self->ioc_, std::move(sock));
    if (self->on_bytes_) self->sess_->on_bytes(self->on_bytes_);
    if (self->on_bp_) self->sess_->on_backpressure(self->on_bp_);
    self->sess_->on_close([self] {
      self->sess_.reset();
      self->state_ = LinkState::Listening;
      self->notify_state();
      self->do_accept();
    });
    self->state_ = LinkState::Connected;
    self->notify_state();
    self->sess_->start();
  });
}

void TcpServer::notify_state() {
  if (on_state_) on_state_(state_);
}
