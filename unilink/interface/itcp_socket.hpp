#pragma once

#include <boost/asio.hpp>
#include <functional>

namespace unilink {
namespace interface {

namespace net = boost::asio;

/**
 * @brief An interface abstracting Boost.Asio's tcp::socket for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class TcpSocketInterface {
 public:
  virtual ~TcpSocketInterface() = default;

  virtual void async_read_some(
      const net::mutable_buffer& buffer,
      std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;
  virtual void async_write(
      const net::const_buffer& buffer,
      std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;
  virtual void shutdown(net::ip::tcp::socket::shutdown_type what, boost::system::error_code& ec) = 0;
  virtual void close(boost::system::error_code& ec) = 0;
  virtual net::ip::tcp::endpoint remote_endpoint(boost::system::error_code& ec) const = 0;
};

}  // namespace interface
}  // namespace unilink
