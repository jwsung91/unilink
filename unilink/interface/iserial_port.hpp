#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <string>

namespace unilink {
namespace interface {

namespace net = boost::asio;

/**
 * @brief An interface abstracting Boost.Asio's serial_port for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class SerialPortInterface {
 public:
  virtual ~SerialPortInterface() = default;

  virtual void open(const std::string& device,
                    boost::system::error_code& ec) = 0;
  virtual bool is_open() const = 0;
  virtual void close(boost::system::error_code& ec) = 0;

  virtual void set_option(const net::serial_port_base::baud_rate& option,
                          boost::system::error_code& ec) = 0;
  virtual void set_option(const net::serial_port_base::character_size& option,
                          boost::system::error_code& ec) = 0;
  virtual void set_option(const net::serial_port_base::stop_bits& option,
                          boost::system::error_code& ec) = 0;
  virtual void set_option(const net::serial_port_base::parity& option,
                          boost::system::error_code& ec) = 0;
  virtual void set_option(const net::serial_port_base::flow_control& option,
                          boost::system::error_code& ec) = 0;

  virtual void async_read_some(
      const net::mutable_buffer& buffer,
      std::function<void(const boost::system::error_code&, std::size_t)>
          handler) = 0;
  virtual void async_write(
      const net::const_buffer& buffer,
      std::function<void(const boost::system::error_code&, std::size_t)>
          handler) = 0;
};

}  // namespace interface
}  // namespace unilink