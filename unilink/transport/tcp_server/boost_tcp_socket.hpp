#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "unilink/interface/itcp_socket.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

/**
 * @brief Boost.Asio implementation of ITcpSocket interface.
 * This is the real implementation used in production.
 */
class BoostTcpSocket : public interface::ITcpSocket {
 public:
  explicit BoostTcpSocket(tcp::socket sock);
  ~BoostTcpSocket() override = default;

  void async_read_some(
      const net::mutable_buffer& buffer,
      std::function<void(const boost::system::error_code&, std::size_t)> handler) override;
  void async_write(
      const net::const_buffer& buffer,
      std::function<void(const boost::system::error_code&, std::size_t)> handler) override;
  void shutdown(tcp::socket::shutdown_type what, boost::system::error_code& ec) override;
  void close(boost::system::error_code& ec) override;
  tcp::endpoint remote_endpoint(boost::system::error_code& ec) const override;

 private:
  tcp::socket socket_;
};

}  // namespace transport
}  // namespace unilink
