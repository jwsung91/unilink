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
#include "unilink/interface/itcp_socket.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace std::chrono_literals;

namespace {

namespace net = boost::asio;
using tcp = net::ip::tcp;

}  // namespace

TEST(TransportTcpServerSessionTest, QueueLimitClosesSession) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
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
  session->async_write_copy(memory::ConstByteSpan(huge.data(), huge.size()));

  ioc.run_for(50ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

TEST(TransportTcpServerSessionTest, MoveWriteRespectsQueueLimit) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
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
  auto work = net::make_work_guard(ioc);
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

TEST(TransportTcpServerSessionTest, BackpressureReliefAfterDrain) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  size_t bp_threshold = 1024;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::vector<size_t> events;
  session->on_backpressure([&](size_t queued) { events.push_back(queued); });

  session->start();
  EXPECT_TRUE(session->alive());

  std::vector<uint8_t> payload(bp_threshold * 2, 0xDD);  // exceed threshold, far below limit
  session->async_write_copy(memory::ConstByteSpan(payload.data(), payload.size()));

  ioc.run_for(50ms);

  ASSERT_GE(events.size(), 2u);
  EXPECT_GE(events.front(), bp_threshold);
  EXPECT_LE(events.back(), bp_threshold / 2);
}

TEST(TransportTcpServerSessionTest, OnBytesExceptionClosesSession) {
  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  size_t bp_threshold = 1024;

  auto socket = std::make_unique<FakeTcpSocket>(ioc);
  auto* socket_raw = socket.get();
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(socket), bp_threshold);

  std::atomic<bool> closed{false};
  session->on_close([&]() { closed = true; });
  session->on_bytes([](memory::ConstByteSpan) { throw std::runtime_error("boom"); });

  session->start();
  // Allow start_read to register handler
  while (!socket_raw->has_handler()) {
    ioc.run_for(std::chrono::milliseconds(1));
  }
  EXPECT_TRUE(session->alive());

  // Trigger read handler to invoke throwing callback
  socket_raw->emit_read(4);

  ioc.run_for(50ms);

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}
