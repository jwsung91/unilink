#include "unilink/transport/tcp_server/boost_tcp_acceptor.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

BoostTcpAcceptor::BoostTcpAcceptor(net::io_context& ioc) : acceptor_(ioc) {}

void BoostTcpAcceptor::open(const net::ip::tcp& protocol, boost::system::error_code& ec) {
  acceptor_.open(protocol, ec);
}

void BoostTcpAcceptor::bind(const net::ip::tcp::endpoint& endpoint, boost::system::error_code& ec) {
  acceptor_.bind(endpoint, ec);
}

void BoostTcpAcceptor::listen(int backlog, boost::system::error_code& ec) { acceptor_.listen(backlog, ec); }

bool BoostTcpAcceptor::is_open() const { return acceptor_.is_open(); }

void BoostTcpAcceptor::close(boost::system::error_code& ec) { acceptor_.close(ec); }

void BoostTcpAcceptor::async_accept(
    std::function<void(const boost::system::error_code&, net::ip::tcp::socket)> handler) {
  acceptor_.async_accept(std::move(handler));
}

}  // namespace transport
}  // namespace unilink
