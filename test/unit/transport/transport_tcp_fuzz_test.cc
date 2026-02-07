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

#include "fake_tcp_socket.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink;
using namespace unilink::transport;

namespace {
namespace net = boost::asio;
using tcp = net::ip::tcp;
}  // namespace

TEST(TransportTcpFuzzTest, FuzzingData) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), 1024);

  session->start();
  // Allow start_read to register handler
  while (!socket_raw->has_handler()) {
    ioc.run_for(std::chrono::milliseconds(1));
  }

  std::atomic<size_t> bytes_received{0};
  session->on_bytes([&](memory::ConstByteSpan) {
    // Process data normally
  });

  // Simulate fuzzy data
  socket_raw->emit_read(1);
  socket_raw->emit_read(10);
  socket_raw->emit_read(100);

  ioc.run_for(std::chrono::milliseconds(50));
}

TEST(TransportTcpFuzzTest, MockParserCrash) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), 1024);

  session->start();
  // Allow start_read to register handler
  while (!socket_raw->has_handler()) {
    ioc.run_for(std::chrono::milliseconds(1));
  }

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  session->on_bytes([&](memory::ConstByteSpan span) {
    if (span.size() == 13) {  // Unlucky number triggers crash
      throw std::runtime_error("Protocol violation");
    }
  });

  // Normal data
  socket_raw->emit_read(10);
  ioc.run_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(closed.load());

  // Malformed data triggering exception
  socket_raw->emit_read(13);
  ioc.run_for(std::chrono::milliseconds(10));

  EXPECT_TRUE(closed.load());
}
