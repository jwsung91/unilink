#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "factory.hpp"
#include "ichannel.hpp"
#include "session.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpServerSingle : public IChannel,
                        public std::enable_shared_from_this<TcpServerSingle> {
 public:
  TcpServerSingle(net::io_context& ioc, uint16_t port)
      : ioc_(ioc),
        acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
        backoff_timer_(ioc) {}

  void start() override {
    state_ = LinkState::Listening;
    notify_state();
    do_accept();
  }
  void stop() override {
    if (session_) session_->close();
    boost::system::error_code ec;
    acceptor_.close(ec);
    backoff_timer_.cancel();
    state_ = LinkState::Closed;
    notify_state();
  }

  bool is_connected() const override { return session_ && session_->alive(); }
  LinkState state() const override { return state_; }

  void async_send(Msg m) override {
    if (session_) session_->send(std::move(m));
  }

  std::future<Msg> request(Msg m, std::chrono::milliseconds timeout) override {
    if (session_) return session_->request(std::move(m), timeout);
    std::promise<Msg> p;
    p.set_exception(std::make_exception_ptr(std::runtime_error("no session")));
    return p.get_future();
  }

  void on_receive(OnReceive cb) override { on_rx_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }

 private:
  void do_accept() {
    auto self = shared_from_this();
    acceptor_.async_accept([self](auto ec, tcp::socket sock) {
      if (ec) {
        std::cout << "[server] accept error: " << ec.message() << std::endl;
        self->schedule_backoff();
        return;
      }

      // 새 연결 로그 (원격 주소)
      boost::system::error_code ep_ec;
      auto rep = sock.remote_endpoint(ep_ec);
      if (!ep_ec) {
        std::cout << "[server] accepted " << rep.address().to_string() << ":"
                  << rep.port() << std::endl;
      } else {
        std::cout << "[server] accepted (endpoint unknown)" << std::endl;
      }

      if (self->session_ && self->session_->alive()) {
        std::cout
            << "[server] already has an active session; closing new socket"
            << std::endl;
        boost::system::error_code ec2;
        sock.shutdown(tcp::socket::shutdown_both, ec2);
        sock.close(ec2);
        self->do_accept();
        return;
      }

      // 수신 로그를 기본으로 찍도록 on_rx에 람다 부여
      self->session_ = std::make_shared<Session>(
          self->ioc_, std::move(sock),
          [self](const Msg& m) {
            std::cout << "[server] recv seq=" << m.seq
                      << " bytes=" << m.bytes.size() << std::endl;
            if (self->on_rx_) self->on_rx_(m);  // 앱 콜백이 있으면 함께 호출
          },
          [self]() {
            std::cout << "[server] session closed" << std::endl;
            self->session_.reset();
            self->state_ = LinkState::Listening;
            self->notify_state();
            self->do_accept();
          });

      self->state_ = LinkState::Connected;
      self->notify_state();
      std::cout << "[server] state -> " << to_cstr(self->state_)
                << std::endl;  // 상태 로그(선택)
      self->session_->start();
    });
  }

  void schedule_backoff() {
    state_ = LinkState::Error;
    notify_state();
    auto self = shared_from_this();
    backoff_timer_.expires_after(std::chrono::seconds(backoff_sec_));
    backoff_timer_.async_wait([self](auto ec) {
      if (!ec) self->do_accept();
    });
    backoff_sec_ = std::min(backoff_sec_ * 2, 30u);
  }

  void notify_state() {
    if (on_state_) on_state_(state_);
  }

 private:
  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<Session> session_;
  net::steady_timer backoff_timer_;
  unsigned backoff_sec_ = 1;

  OnReceive on_rx_;
  OnState on_state_;
  LinkState state_ = LinkState::Idle;
};

// Factory
std::shared_ptr<IChannel> make_server_single(boost::asio::io_context& ioc,
                                             uint16_t port) {
  return std::make_shared<TcpServerSingle>(ioc, port);
}
