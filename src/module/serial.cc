#include "module/serial.hpp"

#include <iostream>

namespace net = boost::asio;

Serial::Serial(net::io_context& ioc, const SerialConfig& cfg)  // NOLINT
    : ioc_(ioc), port_(ioc), cfg_(cfg), retry_timer_(ioc) {
  rx_.resize(cfg_.read_chunk);
}

void Serial::start() {
  state_ = LinkState::Connecting;
  notify_state();
  open_and_configure();
}

void Serial::stop() {
  retry_timer_.cancel();
  close_port();
  state_ = LinkState::Closed;
  notify_state();
}

bool Serial::is_connected() const { return opened_; }

void Serial::async_write_copy(const uint8_t* data, size_t n) {
  std::vector<uint8_t> copy(data, data + n);
  net::post(ioc_, [self = shared_from_this(), buf = std::move(copy)]() mutable {
    self->queued_bytes_ += buf.size();
    self->tx_.emplace_back(std::move(buf));
    if (self->on_bp_ && self->queued_bytes_ > self->bp_high_)
      self->on_bp_(self->queued_bytes_);
    if (!self->writing_) self->do_write();
  });
}

void Serial::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void Serial::on_state(OnState cb) { on_state_ = std::move(cb); }
void Serial::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void Serial::open_and_configure() {
  boost::system::error_code ec;
  port_.open(cfg_.device, ec);
  if (ec) {
    schedule_retry("open", ec);
    return;
  }

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
  std::cout << ts_now() << "[serial] opened " << cfg_.device << " @"
            << cfg_.baud_rate << "\n";
  start_read();
}

void Serial::start_read() {
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

void Serial::do_write() {
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

void Serial::handle_error(const char* where,
                          const boost::system::error_code& ec) {
  std::cout << ts_now() << "[serial] " << where << " error: " << ec.message()
            << "\n";
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

void Serial::schedule_retry(const char* where,
                            const boost::system::error_code&) {
  std::cout << ts_now() << "[serial] retry after "
            << (cfg_.retry_interval_ms / 1000.0) << "s (fixed) at " << where
            << "\n";
  auto self = shared_from_this();
  retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
  retry_timer_.async_wait([self](auto e) {
    if (!e) self->open_and_configure();
  });
}

void Serial::close_port() {
  boost::system::error_code ec;
  if (port_.is_open()) port_.close(ec);
}

void Serial::notify_state() {
  if (on_state_) on_state_(state_);
}
