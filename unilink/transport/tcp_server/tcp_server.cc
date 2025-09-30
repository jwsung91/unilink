#include "unilink/transport/tcp_server/tcp_server.hpp"

#include <iostream>

#include "unilink/transport/tcp_server/boost_tcp_acceptor.hpp"
#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using namespace interface;
using namespace common;
using namespace config;
using tcp = net::ip::tcp;

TcpServer::TcpServer(const TcpServerConfig& cfg)
    : owned_ioc_(nullptr),
      owns_ioc_(false),
      ioc_(common::IoContextManager::instance().get_context()), 
      acceptor_(nullptr), 
      cfg_(cfg) {
  // Create acceptor after all members are initialized
  try {
    acceptor_ = std::make_unique<BoostTcpAcceptor>(ioc_);
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to create TCP acceptor: " + std::string(e.what()));
  }
}

TcpServer::TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                     net::io_context& ioc)
    : owned_ioc_(nullptr),
      owns_ioc_(false),
      ioc_(ioc), 
      acceptor_(std::move(acceptor)), 
      cfg_(cfg) {
  // Ensure acceptor is properly initialized
  if (!acceptor_) {
    throw std::runtime_error("Failed to create TCP acceptor");
  }
}

TcpServer::~TcpServer() {
  // Ensure proper cleanup even if stop() wasn't called explicitly
  if (state_ != LinkState::Closed) {
    stop();
  }
  
  // No need to clean up io_context as it's shared and managed by IoContextManager
}

void TcpServer::start() {
  if (!acceptor_) {
    std::cout << ts_now() << "[server] start error: acceptor is null" << std::endl;
    state_ = LinkState::Error;
    notify_state();
    return;
  }
  
  if (owns_ioc_) {
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }
  net::post(ioc_, [this] {
    boost::system::error_code ec;
    acceptor_->open(tcp::v4(), ec);
    if (!ec) {
      acceptor_->bind(tcp::endpoint(tcp::v4(), cfg_.port), ec);
    }
    if (!ec) {
      acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
    }
    if (ec) {
      std::cout << ts_now() << "[server] bind error: " << ec.message() << std::endl;
      state_ = LinkState::Error;
      notify_state();
      return;
    }
    state_ = LinkState::Listening;
    notify_state();
    do_accept();
  });
}

void TcpServer::stop() {
  if (owns_ioc_ && ioc_thread_.joinable()) {
    net::post(ioc_, [this] {
      boost::system::error_code ec;
      if (acceptor_ && acceptor_->is_open()) {
        acceptor_->close(ec);
      }
      if (sess_) sess_.reset();
    });
    ioc_.stop();
    ioc_thread_.join();
    // Reset io_context to clear any remaining work
    ioc_.restart();
  } else {
    // If server was never started, just clean up
    boost::system::error_code ec;
    if (acceptor_ && acceptor_->is_open()) {
      acceptor_->close(ec);
    }
    if (sess_) sess_.reset();
  }
  state_ = LinkState::Closed;
  // Don't call notify_state() in stop() as it may cause issues with callbacks
  // during destruction
}

bool TcpServer::is_connected() const { return sess_ && sess_->alive(); }

void TcpServer::async_write_copy(const uint8_t* data, size_t size) {
  if (sess_ && sess_->alive()) {
    sess_->async_write_copy(data, size);
  }
  // If no session or session is not alive, the write is silently dropped
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
  if (!acceptor_ || !acceptor_->is_open()) return;
  
  auto self = shared_from_this();
  acceptor_->async_accept([self](auto ec, tcp::socket sock) {
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
  if (on_state_) {
    try {
      on_state_(state_);
    } catch (const std::exception& e) {
      std::cerr << "TcpServer state callback error: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "TcpServer state callback: Unknown error occurred" << std::endl;
    }
  }
}

}  // namespace transport
}  // namespace unilink