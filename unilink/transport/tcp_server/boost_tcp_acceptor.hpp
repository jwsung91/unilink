#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "unilink/interface/itcp_acceptor.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

/**
 * @brief Boost.Asio implementation of ITcpAcceptor interface.
 * This is the real implementation used in production.
 */
class BoostTcpAcceptor : public interface::ITcpAcceptor {
 public:
  explicit BoostTcpAcceptor(net::io_context& ioc);
  ~BoostTcpAcceptor() override = default;

  void open(const net::ip::tcp& protocol, boost::system::error_code& ec) override;
  void bind(const net::ip::tcp::endpoint& endpoint, boost::system::error_code& ec) override;
  void listen(int backlog, boost::system::error_code& ec) override;
  bool is_open() const override;
  void close(boost::system::error_code& ec) override;

  void async_accept(
      std::function<void(const boost::system::error_code&, net::ip::tcp::socket)> handler) override;

 private:
  net::ip::tcp::acceptor acceptor_;
};

}  // namespace transport
}  // namespace unilink
