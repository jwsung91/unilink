
#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <vector>

#include "factory.hpp"
#include "ichannel.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpClient : public IChannel,
                  public std::enable_shared_from_this<TcpClient> {
 public:
  TcpClient(net::io_context& ioc, std::string host, uint16_t port)
      : ioc_(ioc),
        resolver_(ioc),
        socket_(ioc),
        host_(std::move(host)),
        port_(port),
        retry_timer_(ioc) {}

  void start() override {
    state_ = LinkState::Connecting;
    notify_state();
    do_resolve_connect();
  }

  void stop() override {
    retry_timer_.cancel();
    close_socket();
    state_ = LinkState::Closed;
    notify_state();
  }

  bool is_connected() const override { return connected_; }

  void async_write_copy(const uint8_t* data, size_t size) override {
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

  void on_bytes(OnBytes cb) override { on_bytes_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) override { on_bp_ = std::move(cb); }

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
          net::async_connect(self->socket_, results,
                             [self](auto ec2, const auto&) {
                               if (ec2) {
                                 self->schedule_retry();
                                 return;
                               }
                               self->connected_ = true;
                               self->state_ = LinkState::Connected;
                               self->notify_state();
                               // log endpoint
                               boost::system::error_code ep_ec;
                               auto rep = self->socket_.remote_endpoint(ep_ec);
                               if (!ep_ec) {
                                 std::cout << "[client] connected to "
                                           << rep.address().to_string() << ":"
                                           << rep.port() << std::endl;
                               }
                               self->start_read();
                             });
        });
  }

  void schedule_retry() {
    connected_ = false;
    state_ = LinkState::Connecting;
    notify_state();
    std::cout << "[client] reconnect in " << backoff_sec_ << "s" << std::endl;
    auto self = shared_from_this();
    retry_timer_.expires_after(std::chrono::seconds(backoff_sec_));
    retry_timer_.async_wait([self](auto ec) {
      if (!ec) self->do_resolve_connect();
    });
    backoff_sec_ = std::min<unsigned>(backoff_sec_ * 2, 30);
  }

  void start_read() {
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
                         self->handle_close();
                         return;
                       }
                       self->tx_.pop_front();
                       self->do_write();
                     });
  }

  void handle_close() {
    connected_ = false;
    close_socket();
    state_ = LinkState::Connecting;
    notify_state();
    schedule_retry();
  }

  void close_socket() {
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);
  }

  void notify_state() {
    if (on_state_) on_state_(state_);
  }

 private:
  net::io_context& ioc_;
  tcp::resolver resolver_;
  tcp::socket socket_;
  std::string host_;
  uint16_t port_;
  net::steady_timer retry_timer_;
  unsigned backoff_sec_ = 1;

  std::array<uint8_t, 4096> rx_{};

  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  const size_t bp_high_ = 1 << 20;  // 1 MiB

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  bool connected_ = false;
  LinkState state_ = LinkState::Idle;
};

// Factory
std::shared_ptr<IChannel> make_tcp_client(net::io_context& ioc,
                                          const std::string& host,
                                          uint16_t port) {
  return std::make_shared<TcpClient>(ioc, host, port);
}
