#pragma once

#include "unilink/interface/iserial_port.hpp"

namespace unilink {
namespace transport {

class BoostSerialPort : public interface::SerialPortInterface {
 public:
  explicit BoostSerialPort(net::io_context& ioc) : port_(ioc) {}

  void open(const std::string& device, boost::system::error_code& ec) override { port_.open(device, ec); }
  bool is_open() const override { return port_.is_open(); }
  void close(boost::system::error_code& ec) override { port_.close(ec); }

  void set_option(const net::serial_port_base::baud_rate& option, boost::system::error_code& ec) override {
    port_.set_option(option, ec);
  }
  void set_option(const net::serial_port_base::character_size& option, boost::system::error_code& ec) override {
    port_.set_option(option, ec);
  }
  void set_option(const net::serial_port_base::stop_bits& option, boost::system::error_code& ec) override {
    port_.set_option(option, ec);
  }
  void set_option(const net::serial_port_base::parity& option, boost::system::error_code& ec) override {
    port_.set_option(option, ec);
  }
  void set_option(const net::serial_port_base::flow_control& option, boost::system::error_code& ec) override {
    port_.set_option(option, ec);
  }

  void async_read_some(const net::mutable_buffer& buffer,
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    port_.async_read_some(buffer, std::move(handler));
  }

  void async_write(const net::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    net::async_write(port_, buffer, std::move(handler));
  }

 private:
  net::serial_port port_;
};

}  // namespace transport
}  // namespace unilink