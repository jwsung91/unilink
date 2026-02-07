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
<<<<<<< HEAD
=======
#include <vector>
>>>>>>> origin/main

#include "fake_tcp_socket.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink;
using namespace unilink::transport;
<<<<<<< HEAD

namespace {
namespace net = boost::asio;
}

TEST(TransportTcpTimeoutTest, ReadErrorReset) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), 1024);
=======
using unilink::test::FakeTcpSocket;
using namespace std::chrono_literals;

namespace {

namespace net = boost::asio;

TEST(TransportTcpTimeoutTest, ReadErrorReset) {
  net::io_context ioc;
  auto work_guard = net::make_work_guard(ioc);
  size_t bp_threshold = 1024;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);
>>>>>>> origin/main

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  session->start();
<<<<<<< HEAD
  // Allow start_read to register handler
  while (!socket_raw->has_handler()) {
    ioc.run_for(std::chrono::milliseconds(1));
  }
  EXPECT_TRUE(session->alive());

  // Simulate read error
  socket_raw->emit_read(0, boost::asio::error::connection_reset);

  ioc.run_for(std::chrono::milliseconds(10));
=======
  ioc.run_for(5ms);  // Process start_read
  EXPECT_TRUE(session->alive());

  // Simulate connection reset
  socket_raw->emit_read(0, boost::asio::error::connection_reset);

  ioc.run_for(5ms);
>>>>>>> origin/main

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

TEST(TransportTcpTimeoutTest, CancelHandling) {
  net::io_context ioc;
<<<<<<< HEAD
  auto work = net::make_work_guard(ioc);
  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), 1024);

  session->start();
  EXPECT_TRUE(session->alive());

  // Manually cancel
  session->cancel();

  // Cancel should trigger socket closure path
  ioc.run_for(std::chrono::milliseconds(10));

  EXPECT_FALSE(session->alive());
}
=======
  auto work_guard = net::make_work_guard(ioc);
  size_t bp_threshold = 1024;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });

  session->start();
  ioc.run_for(5ms);  // Process start_read
  EXPECT_TRUE(session->alive());

  // Call cancel
  session->cancel();

  // Simulate the resulting operation_aborted error from the socket
  socket_raw->emit_read(0, boost::asio::error::operation_aborted);

  ioc.run_for(5ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

}  // namespace
>>>>>>> origin/main
