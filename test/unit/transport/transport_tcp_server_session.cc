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

#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <vector>

#include "unilink/interface/itcp_socket.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace std::chrono_literals;

namespace {

namespace net = boost::asio;
using tcp = net::ip::tcp;

class FakeTcpSocket : public interface::TcpSocketInterface {
 public:
  explicit FakeTcpSocket(net::io_context& ioc) : ioc_(ioc) {}

  void async_read_some(const net::mutable_buffer&,
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Keep read pending to simulate active connection
    read_handler_ = std::move(handler);
  }

  void async_write(const net::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    // Simulate successful write
    auto size = buffer.size();
    net::post(ioc_, [handler = std::move(handler), size]() { handler({}, size); });
  }

  void shutdown(tcp::socket::shutdown_type, boost::system::error_code& ec) override { ec.clear(); }

  void close(boost::system::error_code& ec) override { ec.clear(); }

  tcp::endpoint remote_endpoint(boost::system::error_code& ec) const override {
    ec.clear();
    return tcp::endpoint(net::ip::make_address("127.0.0.1"), 12345);
  }

 private:
  net::io_context& ioc_;
  std::function<void(const boost::system::error_code&, std::size_t)> read_handler_;
};

}  // namespace

TEST(TransportTcpServerSessionTest, QueueLimitClosesSession) {
  net::io_context ioc;
  size_t bp_threshold = 1024;  // 1KB
  // Hard cap will be 4 * 1024 = 4KB (or default min if larger, but logic respects relative size)
  // Actually implementation uses std::max(bp * 4, DEFAULT_BACKPRESSURE_THRESHOLD).
  // DEFAULT_BACKPRESSURE_THRESHOLD is likely 1MB.
  // To test effectively, we should send enough data to exceed the calculated limit.
  // Let's assume the limit is at least 1MB. We'll send 10MB to be sure.

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  session->start();
  EXPECT_TRUE(session->alive());

  // Send huge data exceeding any reasonable hard cap (e.g. 10MB)
  std::vector<uint8_t> huge(10 * 1024 * 1024, 0xAA);
  session->async_write_copy(huge.data(), huge.size());

  ioc.run_for(50ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

TEST(TransportTcpServerSessionTest, MoveWriteRespectsQueueLimit) {
  net::io_context ioc;
  size_t bp_threshold = 1024;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  session->start();
  EXPECT_TRUE(session->alive());

  std::vector<uint8_t> huge(10 * 1024 * 1024, 0xBB);
  session->async_write_move(std::move(huge));

  ioc.run_for(50ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

TEST(TransportTcpServerSessionTest, SharedWriteRespectsQueueLimit) {
  net::io_context ioc;
  size_t bp_threshold = 1024;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  session->start();
  EXPECT_TRUE(session->alive());

  auto huge = std::make_shared<const std::vector<uint8_t>>(10 * 1024 * 1024, 0xCC);
  session->async_write_shared(huge);

  ioc.run_for(50ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}