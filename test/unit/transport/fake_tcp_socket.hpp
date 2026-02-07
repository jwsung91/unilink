#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <vector>

#include "unilink/interface/itcp_socket.hpp"

namespace unilink {
namespace test {

class FakeTcpSocket : public unilink::interface::TcpSocketInterface {
 public:
  explicit FakeTcpSocket(boost::asio::io_context& ioc) : ioc_(ioc) {}

  void async_read_some(const boost::asio::mutable_buffer&,
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Keep read pending to simulate active connection
    read_handler_ = std::move(handler);
  }

  void async_write(const boost::asio::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Simulate successful write
    auto size = buffer.size();
    boost::asio::post(ioc_, [handler = std::move(handler), size]() { handler({}, size); });
  }

  void emit_read(std::size_t n = 1, const boost::system::error_code& ec = {}) {
    if (!read_handler_) return;
    auto handler = std::move(read_handler_);
    boost::asio::post(ioc_, [handler = std::move(handler), ec, n]() { handler(ec, n); });
  }

  void shutdown(boost::asio::ip::tcp::socket::shutdown_type, boost::system::error_code& ec) override { ec.clear(); }

  void close(boost::system::error_code& ec) override { ec.clear(); }

  boost::asio::ip::tcp::endpoint remote_endpoint(boost::system::error_code& ec) const override {
    ec.clear();
    return boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 12345);
  }

 private:
  boost::asio::io_context& ioc_;
  std::function<void(const boost::system::error_code&, std::size_t)> read_handler_;
};

}  // namespace test
}  // namespace unilink
