#include "unilink/transport/serial/serial.hpp"

#include <iostream>

#include "unilink/transport/serial/boost_serial_port.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using namespace common;
using namespace config;

Serial::Serial(const SerialConfig& cfg)
    : ioc_(*new net::io_context()),
      owns_ioc_(true),
      cfg_(cfg),
      retry_timer_(ioc_) {
  rx_.resize(cfg_.read_chunk);
  port_ = std::make_unique<BoostSerialPort>(ioc_);
}

// For testing with dependency injection
Serial::Serial(const SerialConfig& cfg,
               std::unique_ptr<interface::ISerialPort> port,
               net::io_context& ioc)
    : ioc_(ioc),
      owns_ioc_(false),
      port_(std::move(port)),
      cfg_(cfg),
      retry_timer_(ioc_) {
  rx_.resize(cfg_.read_chunk);
}

Serial::~Serial() {
  // stop() might have been called already. Ensure we don't double-stop,
  // but do clean up resources if we own them.
  if (state_ != LinkState::Closed) stop();

  if (owns_ioc_) {
    if (ioc_thread_.joinable()) ioc_thread_.join();
    // Ensure io_context is completely stopped before deletion
    ioc_.stop();
    ioc_.restart();
    delete &ioc_;
  }
}

void Serial::start() {
  work_guard_ = std::make_unique<
      net::executor_work_guard<net::io_context::executor_type>>(
      ioc_.get_executor());
  if (owns_ioc_) {
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }
  net::post(ioc_, [this] {
    state_ = LinkState::Connecting;
    notify_state();
    open_and_configure();
  });
}

void Serial::stop() {
  if (state_ != LinkState::Closed) {
    work_guard_->reset();  // Allow the io_context to run out of work.
    net::post(ioc_, [this] {
      // Cancel all pending async operations to unblock the io_context
      retry_timer_.cancel();
      close_port();
      // Post stop() to ensure it's the last thing to run before the context
      // runs out of work.
      ioc_.stop();
    });
    
    // Wait for all async operations to complete
    if (owns_ioc_ && ioc_thread_.joinable()) {
      ioc_thread_.join();
    }
    
    // Reset the io_context to clear any remaining work
    if (owns_ioc_) {
      ioc_.restart();
    }
    
    state_ = LinkState::Closed;
    notify_state();
  }
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
  port_->open(cfg_.device, ec);
  if (ec) {
    handle_error("open", ec);
    return;
  }

  port_->set_option(net::serial_port_base::baud_rate(cfg_.baud_rate), ec);
  port_->set_option(net::serial_port_base::character_size(cfg_.char_size), ec);

  using sb = net::serial_port_base::stop_bits;
  port_->set_option(sb(cfg_.stop_bits == 2 ? sb::two : sb::one), ec);

  using pa = net::serial_port_base::parity;
  pa::type p = pa::none;
  if (cfg_.parity == SerialConfig::Parity::Even)
    p = pa::even;
  else if (cfg_.parity == SerialConfig::Parity::Odd)
    p = pa::odd;
  port_->set_option(pa(p), ec);

  using fc = net::serial_port_base::flow_control;
  fc::type f = fc::none;
  if (cfg_.flow == SerialConfig::Flow::Software)
    f = fc::software;
  else if (cfg_.flow == SerialConfig::Flow::Hardware)
    f = fc::hardware;
  port_->set_option(fc(f), ec);

  std::cout << ts_now() << "[serial] opened " << cfg_.device << " @"
            << cfg_.baud_rate << "\n";
  start_read();

  opened_ = true;
  state_ = LinkState::Connected;
  notify_state();
}

void Serial::start_read() {
  auto self = shared_from_this();
  port_->async_read_some(net::buffer(rx_.data(), rx_.size()),
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
  port_->async_write(net::buffer(tx_.front()), [self](auto ec, std::size_t n) {
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
  // EOF는 실제 에러가 아니므로, 다시 읽기를 시작합니다.
  if (ec == boost::asio::error::eof) {
    start_read();
    return;
  }

  std::cout << ts_now() << "[serial] " << where << " error: " << ec.message()
            << "\n";
  if (cfg_.reopen_on_error) {
    opened_ = false;
    close_port();
    state_ = LinkState::Connecting;
    notify_state();
    schedule_retry(where, ec);
  } else {
    opened_ = false;
    close_port();
    state_ = LinkState::Error;
    notify_state();
  }
}

void Serial::schedule_retry(const char* where,
                            const boost::system::error_code& ec) {
  std::cout << ts_now() << "[serial] retry after "
            << (cfg_.retry_interval_ms / 1000.0) << "s (fixed) at " << where
            << " (" << ec.message() << ")" << "\n";
  auto self = shared_from_this();
  retry_timer_.expires_after(std::chrono::milliseconds(cfg_.retry_interval_ms));
  retry_timer_.async_wait([self](auto e) {
    if (!e) self->open_and_configure();
  });
}

void Serial::close_port() {
  boost::system::error_code ec;
  if (port_ && port_->is_open()) {
    // For serial_port, close() cancels pending asynchronous operations. The
    // read handler will be called with an error.
    port_->close(ec);
  }
}

void Serial::notify_state() {
  if (on_state_) on_state_(state_);
}

}  // namespace transport
}  // namespace unilink