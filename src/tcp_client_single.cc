#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "factory.hpp"
#include "ichannel.hpp"
#include "session.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpClientSingle : public IChannel,
                        public std::enable_shared_from_this<TcpClientSingle> {
 public:
  TcpClientSingle(net::io_context& ioc, std::string host, uint16_t port)
      : ioc_(ioc),
        resolver_(ioc),
        host_(std::move(host)),
        port_(port),
        retry_timer_(ioc) {}

  void start() override {
    state_ = LinkState::Connecting;
    notify_state();
    do_resolve_connect();
  }

  void stop() override {
    if (session_) session_->close();
    retry_timer_.cancel();
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
    p.set_exception(
        std::make_exception_ptr(std::runtime_error("not connected")));
    return p.get_future();
  }

  void on_receive(OnReceive cb) override { on_rx_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }

 private:
  void do_resolve_connect() {
    auto self = shared_from_this();
    resolver_.async_resolve(
        host_, std::to_string(port_),
        [self](auto ec, tcp::resolver::results_type results) {
          if (ec) {
            self->schedule_retry();
            return;
          }

          auto sock = tcp::socket(self->ioc_);
          // NOTE: mutable 캡처로 소켓 이동을 안전하게 처리
          boost::asio::async_connect(
              sock, results,
              [self, s = std::move(sock)](auto ec2, const auto&) mutable {
                if (ec2) {
                  self->schedule_retry();
                  return;
                }

                // ✅ 연결 성공: 원격 엔드포인트 로그
                boost::system::error_code ep_ec;
                auto rep = s.remote_endpoint(ep_ec);
                if (!ep_ec) {
                  std::cout << "[client] connected to "
                            << rep.address().to_string() << ":" << rep.port()
                            << std::endl;
                } else {
                  std::cout << "[client] connected (endpoint unknown)"
                            << std::endl;
                }

                // ✅ 세션 생성: 기본 수신/종료 로그 내장
                self->session_ = std::make_shared<Session>(
                    self->ioc_, std::move(s),
                    // on_receive: 서버가 push 보낼 때(요청-응답이 아닌 일반
                    // 수신)
                    [self](const Msg& m) {
                      std::cout << "[client] recv push: seq=" << m.seq
                                << " bytes=" << m.bytes.size() << std::endl;
                      if (self->on_rx_) self->on_rx_(m);
                    },
                    // on_close: 세션 종료 로그 + 재접속
                    [self]() {
                      std::cout << "[client] session closed" << std::endl;
                      self->session_.reset();
                      self->state_ = LinkState::Connecting;
                      self->notify_state();
                      self->schedule_retry();
                    });

                self->state_ = LinkState::Connected;
                self->notify_state();
                self->session_->start();
              });
        });
  }

  void schedule_retry() {
    self->state_ = LinkState::Connecting;
    self->notify_state();  // (기존)
    // 아래는 함수 맨 앞/뒤 아무 데나 OK. 가독성을 위해 expires_after 앞에
    // 두세요.
    std::cout << "[client] reconnect in " << backoff_sec_ << "s" << std::endl;

    auto self = shared_from_this();
    retry_timer_.expires_after(std::chrono::seconds(backoff_sec_));
    retry_timer_.async_wait([self](auto ec) {
      if (!ec) self->do_resolve_connect();
    });
    backoff_sec_ = std::min(backoff_sec_ * 2, 30u);
  }

  void notify_state() {
    if (on_state_) on_state_(state_);
  }

 private:
  net::io_context& ioc_;
  tcp::resolver resolver_;
  std::shared_ptr<Session> session_;
  std::string host_;
  uint16_t port_;
  net::steady_timer retry_timer_;
  unsigned backoff_sec_ = 1;

  OnReceive on_rx_;
  OnState on_state_;
  LinkState state_ = LinkState::Idle;
};

// Factory
std::shared_ptr<IChannel> make_client_single(boost::asio::io_context& ioc,
                                             const std::string& host,
                                             uint16_t port) {
  return std::make_shared<TcpClientSingle>(ioc, host, port);
}
