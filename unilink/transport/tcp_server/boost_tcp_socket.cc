#include "unilink/transport/tcp_server/boost_tcp_socket.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

BoostTcpSocket::BoostTcpSocket(tcp::socket sock) : socket_(std::move(sock)) {}

void BoostTcpSocket::async_read_some(
    const net::mutable_buffer& buffer,
    std::function<void(const boost::system::error_code&, std::size_t)> handler) {
  socket_.async_read_some(buffer, std::move(handler));
}

void BoostTcpSocket::async_write(
    const net::const_buffer& buffer,
    std::function<void(const boost::system::error_code&, std::size_t)> handler) {
  net::async_write(socket_, buffer, std::move(handler));
}

void BoostTcpSocket::shutdown(tcp::socket::shutdown_type what, boost::system::error_code& ec) {
  socket_.shutdown(what, ec);
}

void BoostTcpSocket::close(boost::system::error_code& ec) {
  socket_.close(ec);
}

tcp::endpoint BoostTcpSocket::remote_endpoint(boost::system::error_code& ec) const {
  return socket_.remote_endpoint(ec);
}

}  // namespace transport
}  // namespace unilink
