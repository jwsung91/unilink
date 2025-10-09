#pragma once

#include <boost/asio.hpp>
#include <functional>

namespace unilink {
namespace interface {

namespace net = boost::asio;

/**
 * @brief An interface abstracting Boost.Asio's tcp::acceptor for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class TcpAcceptorInterface {
 public:
  virtual ~TcpAcceptorInterface() = default;

  virtual void open(const net::ip::tcp& protocol, boost::system::error_code& ec) = 0;
  virtual void bind(const net::ip::tcp::endpoint& endpoint, boost::system::error_code& ec) = 0;
  virtual void listen(int backlog, boost::system::error_code& ec) = 0;
  virtual bool is_open() const = 0;
  virtual void close(boost::system::error_code& ec) = 0;

  virtual void async_accept(std::function<void(const boost::system::error_code&, net::ip::tcp::socket)> handler) = 0;
};

}  // namespace interface
}  // namespace unilink
