// src/transport/serial_channel.cc (신규)
#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <vector>

#include "factory.hpp"
#include "ichannel.hpp"

namespace net = boost::asio;

class SerialChannel : public IChannel,
                      public std::enable_shared_from_this<SerialChannel> {
 public:
  SerialChannel(net::io_context& ioc, std::string device, SerialConfig cfg)
      : ioc_(ioc),
        port_(ioc),
        device_(std::move(device)),
        cfg_(cfg),
        retry_timer_(ioc) {
    rx_.resize(cfg_.read_chunk);
  }

  void start() override {
    state_ = LinkState::Connecting;
    notify_state();
    open_and_configure();
  }

  void stop() override {
    retry_timer_.cancel();
    close_port();
    state_ = LinkState::Closed;
    notify_state();
  }

  bool is_connected() const override { return opened_; }

  void async_write_copy(const uint8_t* data, size_t n) override {
    std::vector<uint8_t> copy(data, data + n);
    net::post(ioc_,
              [self = shared_from_this(), buf = std::move(copy)]() mutable {
                self->queued_bytes_ += buf.size();
                self->tx_.emplace_back(std::move(buf));
                if (self->on_bp_ && self->queued_bytes_ > self->bp_high_)
                  self->on_bp_(self->queued_bytes_);
                if (!self->writing_) self->do_write();
              });
  }

  void on_bytes(OnBytes cb) override { on_bytes_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) override { on_bp_ = std::move(cb); }

 private:
  void open_and_configure() {
    boost::system::error_code ec;
    port_.open(device_, ec);
    if (ec) {
      schedule_retry("open", ec);
      return;
    }

    // 기본 설정
    port_.set_option(net::serial_port_base::baud_rate(cfg_.baud_rate), ec);
    port_.set_option(net::serial_port_base::character_size(cfg_.char_size), ec);

    using sb = net::serial_port_base::stop_bits;
    port_.set_option(sb(cfg_.stop_bits == 2 ? sb::two : sb::one), ec);

    using pa = net::serial_port_base::parity;
    pa::type p = pa::none;
    if (cfg_.parity == SerialConfig::Parity::Even)
      p = pa::even;
    else if (cfg_.parity == SerialConfig::Parity::Odd)
      p = pa::odd;
    port_.set_option(pa(p), ec);

    using fc = net::serial_port_base::flow_control;
    fc::type f = fc::none;
    if (cfg_.flow == SerialConfig::Flow::Software)
      f = fc::software;
    else if (cfg_.flow == SerialConfig::Flow::Hardware)
      f = fc::hardware;
    port_.set_option(fc(f), ec);

    opened_ = true;
    state_ = LinkState::Connected;
    notify_state();
    std::cout << "[serial] opened " << device_ << " @" << cfg_.baud_rate
              << "\n";
    start_read();
  }

  void start_read() {
    auto self = shared_from_this();
    port_.async_read_some(net::buffer(rx_.data(), rx_.size()),
                          [self](auto ec, std::size_t n) {
                            if (ec) {
                              self->handle_error("read", ec);
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
    net::async_write(port_, net::buffer(tx_.front()),
                     [self](auto ec, std::size_t n) {
                       self->queued_bytes_ -= n;
                       if (ec) {
                         self->handle_error("write", ec);
                         return;
                       }
                       self->tx_.pop_front();
                       self->do_write();
                     });
  }

  void handle_error(const char* where, const boost::system::error_code& ec) {
    std::cout << "[serial] " << where << " error: " << ec.message() << "\n";
    opened_ = false;
    close_port();
    state_ = LinkState::Connecting;
    notify_state();
    if (cfg_.reopen_on_error)
      schedule_retry(where, ec);
    else {
      state_ = LinkState::Error;
      notify_state();
    }
  }

  void schedule_retry(const char* where, const boost::system::error_code&) {
    std::cout << "[serial] retry after " << backoff_sec_ << "s (" << where
              << ")\n";
    auto self = shared_from_this();
    retry_timer_.expires_after(std::chrono::seconds(backoff_sec_));
    retry_timer_.async_wait([self](auto e) {
      if (!e) self->open_and_configure();
    });
    backoff_sec_ = std::min<unsigned>(backoff_sec_ * 2, 30);
  }

  void close_port() {
    boost::system::error_code ec;
    if (port_.is_open()) port_.close(ec);
  }

  void notify_state() {
    if (on_state_) on_state_(state_);
  }

 private:
  net::io_context& ioc_;
  net::serial_port port_;
  std::string device_;
  SerialConfig cfg_;
  net::steady_timer retry_timer_;
  unsigned backoff_sec_ = 1;

  std::vector<uint8_t> rx_;
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queued_bytes_ = 0;
  const size_t bp_high_ = 1 << 20;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  bool opened_ = false;
  LinkState state_ = LinkState::Idle;
};

// Factory
std::shared_ptr<IChannel> make_serial_channel(net::io_context& ioc,
                                              const std::string& device,
                                              const SerialConfig& cfg) {
  return std::make_shared<SerialChannel>(ioc, device, cfg);
}
