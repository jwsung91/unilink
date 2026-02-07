<<<<<<< HEAD
/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

=======
>>>>>>> origin/main
#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <memory>
<<<<<<< HEAD

#include "unilink/interface/itcp_socket.hpp"

namespace {

namespace net = boost::asio;
using tcp = net::ip::tcp;

class FakeTcpSocket : public unilink::interface::TcpSocketInterface {
 public:
  explicit FakeTcpSocket(net::io_context& ioc) : ioc_(ioc) {}
  virtual ~FakeTcpSocket() = default;

  void async_read_some(const net::mutable_buffer&,
=======
#include <vector>

#include "unilink/interface/itcp_socket.hpp"

namespace unilink {
namespace test {

class FakeTcpSocket : public unilink::interface::TcpSocketInterface {
 public:
  explicit FakeTcpSocket(boost::asio::io_context& ioc) : ioc_(ioc) {}

  void async_read_some(const boost::asio::mutable_buffer&,
>>>>>>> origin/main
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Keep read pending to simulate active connection
    read_handler_ = std::move(handler);
  }

<<<<<<< HEAD
  void async_write(const net::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Simulate successful write
    auto size = buffer.size();
    net::post(ioc_, [handler = std::move(handler), size]() { handler({}, size); });
=======
  void async_write(const boost::asio::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Simulate successful write
    auto size = buffer.size();
    boost::asio::post(ioc_, [handler = std::move(handler), size]() { handler({}, size); });
>>>>>>> origin/main
  }

  void emit_read(std::size_t n = 1, const boost::system::error_code& ec = {}) {
    if (!read_handler_) return;
    auto handler = std::move(read_handler_);
<<<<<<< HEAD
    net::post(ioc_, [handler = std::move(handler), ec, n]() { handler(ec, n); });
  }

  bool has_handler() const { return !!read_handler_; }

  void shutdown(tcp::socket::shutdown_type, boost::system::error_code& ec) override { ec.clear(); }

  void close(boost::system::error_code& ec) override {
    // Closing a socket cancels all pending operations
    if (read_handler_) {
      auto handler = std::move(read_handler_);
      net::post(ioc_, [handler]() { handler(boost::asio::error::operation_aborted, 0); });
    }
    ec.clear();
  }

  tcp::endpoint remote_endpoint(boost::system::error_code& ec) const override {
    ec.clear();
    return tcp::endpoint(net::ip::make_address("127.0.0.1"), 12345);
  }

 private:
  net::io_context& ioc_;
  std::function<void(const boost::system::error_code&, std::size_t)> read_handler_;
};

}  // namespace
=======
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
>>>>>>> origin/main
