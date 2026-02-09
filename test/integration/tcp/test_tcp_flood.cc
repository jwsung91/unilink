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
  // We use a separate context struct to hold the weak_ptr to the server.
  // This allows the callback to be independent of 'this' (TcpFloodTest instance)
  // and avoids use-after-free if the test fixture is destroyed while callbacks are pending.
  struct TestContext {
    std::weak_ptr<wrapper::TcpServer> server_weak;
  };
  auto ctx = std::make_shared<TestContext>();

  server_ = unilink::tcp_server(test_port_)
                .unlimited_clients()
                .on_multi_data([ctx](size_t client_id, const std::string& data) {
                  if (auto server = ctx->server_weak.lock()) {
                    server->send_to_client(client_id, data);
                  }
                })
                .build();

  // Assign the weak pointer now that server_ is created
  ctx->server_weak = server_;

  server_->start();
  ASSERT_TRUE(unilink::test::TestUtils::waitForCondition([this]() { return server_->is_listening(); }, 2000));

  // 2. Connect Client
  net::io_context ioc;
  tcp::socket socket(ioc);
  try {
    socket.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), test_port_));
  } catch (const std::exception& e) {
    FAIL() << "Failed to connect: " << e.what();
  }

  // 3. Flood Data
  // Increased to 10MB to force potential backpressure trigger.
  // The server has a default backpressure limit (~4MB). Sending 10MB and sleeping
  // will likely cause the server to queue data beyond the limit and disconnect.
  // This is expected and verifies the buffering/protection logic.
  const size_t flood_size = 10 * 1024 * 1024;  // 10MB
  const size_t chunk_size = 1024 * 1024;       // 1MB chunk
  const int chunks = flood_size / chunk_size;
  std::string chunk(chunk_size, 'X');

  // Disable Nagle's algorithm for faster sending
  socket.set_option(tcp::no_delay(true));

  // Send chunks
  boost::system::error_code ec;
  bool disconnection_detected = false;

  for (int i = 0; i < chunks; ++i) {
    net::write(socket, net::buffer(chunk), ec);
    if (ec) {
      // If we get an error while writing (e.g. broken pipe because server closed early),
      // that is also a sign of backpressure kicking in.
      std::cout << "Write error (expected if server disconnects): " << ec.message() << std::endl;
      disconnection_detected = true;
      break;
    }
  }

  // 4. Wait a bit to let server queue up responses
  // This allows the socket buffer to fill up and backpressure logic to trigger
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // 5. Read back
  size_t total_read = 0;
  std::vector<char> read_buf(64 * 1024);  // 64KB read buffer

  auto start = std::chrono::steady_clock::now();
  while (total_read < flood_size) {
    // Increase timeout to accommodate potential slowness in CI or large data
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(30)) {
      // If we timed out but got some data, it might be just slow.
      // But 30s is a long time.
      break;
    }

    size_t n = socket.read_some(net::buffer(read_buf), ec);
    if (ec) {
      if (ec == net::error::eof) {
        std::cout << "Server disconnected (backpressure limit reached), read: " << total_read << std::endl;
        disconnection_detected = true;
        break;
      }
      // Connection reset is also possible
      if (ec == net::error::connection_reset) {
        std::cout << "Connection reset (backpressure limit reached), read: " << total_read << std::endl;
        disconnection_detected = true;
        break;
      }
      // On Windows, write error might be "An existing connection was forcibly closed by the remote host"
      // effectively a reset
      if (ec.value() == 10054) {  // WSAECONNRESET
        std::cout << "WSAECONNRESET (backpressure limit reached), read: " << total_read << std::endl;
        disconnection_detected = true;
        break;
      }

      FAIL() << "Read error: " << ec.message();
    }
    total_read += n;
  }

  // Verification:
  // Either we successfully triggered backpressure (disconnection), OR we read everything (no backpressure).
  // Partial reads without disconnection are failure (timeout).
  // Total read 0 is acceptable IF disconnected (e.g. Windows rapid RST).
  if (disconnection_detected) {
    SUCCEED() << "Server disconnected client as expected (backpressure)";
  } else {
    // If we didn't disconnect, we MUST have read everything back.
    EXPECT_EQ(total_read, flood_size) << "Did not receive all data and was not disconnected";
  }

  // Verify server is still alive (listening for new connections)
  // It shouldn't have crashed.
  EXPECT_TRUE(server_->is_listening());
}
