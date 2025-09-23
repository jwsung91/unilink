#include "unilink/transport/tcp_server/tcp_server.hpp"

#include <iostream>

namespace unilink {
namespace transport {

namespace net = boost::asio;

using namespace interface;
using namespace common;
using namespace config;
using tcp = net::ip::tcp;

TcpServer::TcpServer(const TcpServerConfig& cfg)
    : ioc_(), acceptor_(ioc_, tcp::endpoint(tcp::v4(), cfg.port)), cfg_(cfg) {}

void TcpServer::start() {
  ioc_thread_ = std::thread([this] { ioc_.run(); });
  net::post(ioc_, [this] {
    state_ = LinkState::Listening;
    notify_state();
    do_accept();
  });
}

void TcpServer::stop() {
  net::post(ioc_, [this] {
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (sess_) sess_.reset();
  });
  ioc_.stop();
  if (ioc_thread_.joinable()) ioc_thread_.join();
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
      std::cout << ts_now() << "[server] accepted " << rep.address().to_string()
                << ":" << rep.port() << std::endl;
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

}  // namespace transport
}  // namespace unilink