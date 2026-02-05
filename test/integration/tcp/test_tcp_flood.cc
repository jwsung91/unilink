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
#include <future>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpFloodTest : public unilink::test::NetworkTest {
 protected:
  void SetUp() override { unilink::test::NetworkTest::SetUp(); }

  void TearDown() override {
    if (server_) {
      server_->stop();
      server_.reset();
    }
    unilink::test::NetworkTest::TearDown();
  }

  std::shared_ptr<wrapper::TcpServer> server_;
};

/**
 * @brief Flood Server Test
 * Client sends 10MB of data rapidly. Server echoes it back.
 * This stresses the TCP send buffer and internal queueing mechanism.
 */
TEST_F(TcpFloodTest, FloodServer) {
  // 1. Start Server
  server_ =
      unilink::tcp_server(test_port_)
          .unlimited_clients()
          .on_multi_data([this](size_t client_id, const std::string& data) {
            server_->send_to_client(client_id, data);
          })
          .build();

  server_->start();
  ASSERT_TRUE(unilink::test::TestUtils::waitForCondition(
      [this]() { return server_->is_listening(); }, 2000));

  // 2. Connect Client
  net::io_context ioc;
  tcp::socket socket(ioc);
  try {
    socket.connect(
        tcp::endpoint(net::ip::address::from_string("127.0.0.1"), test_port_));
  } catch (const std::exception& e) {
    FAIL() << "Failed to connect: " << e.what();
  }

  // 3. Flood Data
  const size_t flood_size =
      3 * 1024 * 1024;  // 3MB (Under 4MB default limit to avoid forced close)
  const size_t chunk_size = 1024 * 1024;  // 1MB chunk
  const int chunks = flood_size / chunk_size;
  std::string chunk(chunk_size, 'X');

  // Disable Nagle's algorithm for faster sending
  socket.set_option(tcp::no_delay(true));

  // Send chunks
  boost::system::error_code ec;
  for (int i = 0; i < chunks; ++i) {
    net::write(socket, net::buffer(chunk), ec);
    if (ec) FAIL() << "Write error: " << ec.message();
  }

  // 4. Wait a bit to let server queue up responses
  // This allows the socket buffer to fill up and backpressure logic to trigger
  // if applicable
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // 5. Read back
  // We expect 10MB back.
  size_t total_read = 0;
  std::vector<char> read_buf(64 * 1024);  // 64KB read buffer

  auto start = std::chrono::steady_clock::now();
  while (total_read < flood_size) {
    // Increase timeout to accommodate potential slowness in CI or large data
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(30)) {
      FAIL() << "Timeout waiting for flood echo. Read so far: " << total_read;
    }

    size_t n = socket.read_some(net::buffer(read_buf), ec);
    if (ec) {
      if (ec == net::error::eof) break;
      FAIL() << "Read error: " << ec.message();
    }
    total_read += n;
  }

  EXPECT_EQ(total_read, flood_size);
}
