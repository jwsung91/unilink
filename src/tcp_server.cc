
#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "factory.hpp"
#include "ichannel.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpServer : public std::enable_shared_from_this<TcpServer> {
 public:
  using OnBytes = IChannel::OnBytes;
  using OnBackpressure = IChannel::OnBackpressure;
  using OnClose = std::function<void()>;

  TcpServer(net::io_context& ioc, tcp::socket sock)
      : ioc_(ioc), socket_(std::move(sock)) {}

  void start() { start_read(); }

  void async_write_copy(const uint8_t* data, size_t size) {
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

  void on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }
  void on_close(OnClose cb) { on_close_ = std::move(cb); }

  bool alive() const { return alive_; }

 private:
  void start_read() {
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

  void do_write() {
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

  void do_close() {
    if (!alive_) return;
    alive_ = false;
    std::cout << "[server] client disconnected" << std::endl;
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    if (on_close_) on_close_();
  }

 private:
  net::io_context& ioc_;
  tcp::socket socket_;
  std::array<uint8_t, 4096> rx_{};
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  const size_t bp_high_ = 1 << 20;  // 1 MiB

  OnBytes on_bytes_;
  OnBackpressure on_bp_;
  OnClose on_close_;
  bool alive_ = false;
};

class TcpServerSingleTransport
    : public IChannel,
      public std::enable_shared_from_this<TcpServerSingleTransport> {
 public:
  TcpServerSingleTransport(net::io_context& ioc, uint16_t port)
      : ioc_(ioc), acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {}

  void start() override {
    state_ = LinkState::Listening;
    notify_state();
    do_accept();
  }

  void stop() override {
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (sess_) sess_.reset();
    state_ = LinkState::Closed;
    notify_state();
  }

  bool is_connected() const override { return sess_ && sess_->alive(); }

  void async_write_copy(const uint8_t* data, size_t size) override {
    if (sess_) sess_->async_write_copy(data, size);
  }

  void on_bytes(OnBytes cb) override {
    on_bytes_ = std::move(cb);
    if (sess_) sess_->on_bytes(on_bytes_);
  }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) override {
    on_bp_ = std::move(cb);
    if (sess_) sess_->on_backpressure(on_bp_);
  }

 private:
  void do_accept() {
    auto self = shared_from_this();
    acceptor_.async_accept([self](auto ec, tcp::socket sock) {
      if (ec) {
        std::cout << "[server] accept error: " << ec.message() << std::endl;
        self->state_ = LinkState::Error;
        self->notify_state();
        self->do_accept();
        return;
      }
      boost::system::error_code ep_ec;
      auto rep = sock.remote_endpoint(ep_ec);
      if (!ep_ec) {
        std::cout << "[server] accepted " << rep.address().to_string() << ":"
                  << rep.port() << std::endl;
      } else {
        std::cout << "[server] accepted (endpoint unknown)" << std::endl;
      }

      self->sess_ = std::make_shared<TcpServer>(self->ioc_, std::move(sock));
      if (self->on_bytes_) self->sess_->on_bytes(self->on_bytes_);
      if (self->on_bp_) self->sess_->on_backpressure(self->on_bp_);
      self->sess_->on_close([self] {
        // session ended -> return to listening and accept next client
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

  void notify_state() {
    if (on_state_) on_state_(state_);
  }

 private:
  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<TcpServer> sess_;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  LinkState state_ = LinkState::Idle;
};

// Factory
std::shared_ptr<IChannel> make_tcp_server_single(net::io_context& ioc,
                                                 uint16_t port) {
  return std::make_shared<TcpServerSingleTransport>(ioc, port);
}
