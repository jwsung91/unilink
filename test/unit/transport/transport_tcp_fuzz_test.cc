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
#include <random>
#include <vector>

#include "fake_tcp_socket.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink;
using namespace unilink::transport;
using unilink::test::FakeTcpSocket;
using namespace std::chrono_literals;

namespace {

namespace net = boost::asio;

TEST(TransportTcpFuzzTest, FuzzingData) {
  net::io_context ioc;
  auto work_guard = net::make_work_guard(ioc);
  size_t bp_threshold = 65536;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  // Simple echo or no-op parser
  session->on_bytes([&](memory::ConstByteSpan) {
    // Process data normally
  });

  session->start();
  // Allow start_read to register handler
  while (!socket_raw->has_handler()) {
    ioc.run_for(1ms);
  }
  EXPECT_TRUE(session->alive());

  std::mt19937 gen(12345);
  std::uniform_int_distribution<size_t> size_dist(1, 4096);

  // Send 100 random packets
  for (int i = 0; i < 100; ++i) {
    if (!session->alive()) break;

    size_t size = size_dist(gen);
    // Data is random garbage as per FakeTcpSocket implementation discussion

    socket_raw->emit_read(size);
    ioc.run_for(1ms);
  }

  EXPECT_TRUE(session->alive());
  session->stop();
  ioc.run_for(10ms);
  EXPECT_FALSE(session->alive());
}

TEST(TransportTcpFuzzTest, MockParserCrash) {
  net::io_context ioc;
  auto work_guard = net::make_work_guard(ioc);
  size_t bp_threshold = 65536;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  // Mock parser that throws on specific "bad" length
  session->on_bytes([&](memory::ConstByteSpan span) {
    if (span.size() == 13) {  // Unlucky number triggers crash
      throw std::runtime_error("Protocol violation");
    }
  });

  session->start();
  // Allow start_read to register handler
  while (!socket_raw->has_handler()) {
    ioc.run_for(1ms);
  }
  EXPECT_TRUE(session->alive());

  // Send safe data
  socket_raw->emit_read(10);
  ioc.run_for(5ms);
  EXPECT_TRUE(session->alive());

  // Send "malformed" packet
  socket_raw->emit_read(13);
  ioc.run_for(5ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

}  // namespace
