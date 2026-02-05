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
#include <thread>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpAbortTest : public unilink::test::NetworkTest {
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
 * @brief Session Abortion Test
 * Verifies server handles client sending RST gracefully while data might be
 * pending or partial.
 */
TEST_F(TcpAbortTest, SessionAbortion) {
  std::atomic<bool> error_reported{false};
  std::atomic<bool> disconnected{false};
  std::atomic<bool> connected{false};

  // 1. Start Server
  server_ = unilink::tcp_server(test_port_)
                .unlimited_clients()
                .on_multi_connect([&connected](size_t, const std::string&) { connected = true; })
                .on_multi_disconnect([&disconnected](size_t) { disconnected = true; })
                .on_error([&error_reported](const std::string& err) {
                  // Depending on implementation, RST might trigger on_error or just
                  // on_disconnect Usually read error (connection reset by peer)
                  // triggers on_disconnect. But if it was unexpected during read,
                  // maybe error logged. Let's just track if it crashes or not.
                  error_reported = true;
                })
                .build();

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

  // Wait for server to accept
  ASSERT_TRUE(unilink::test::TestUtils::waitForCondition([&connected]() { return connected.load(); }, 5000))
      << "Server did not accept connection";

  // 3. Send Partial Data
  std::string partial_data = "Partial Data...";
  boost::system::error_code ec;
  net::write(socket, net::buffer(partial_data), ec);
  ASSERT_FALSE(ec);

  // 4. Hard Close (RST)
  // To send RST, set SO_LINGER with timeout 0 and close.
  boost::asio::socket_base::linger option(true, 0);
  socket.set_option(option);
  socket.close();

  // 5. Verify Server Handle
  // Wait for disconnect callback
  bool closed_gracefully =
      unilink::test::TestUtils::waitForCondition([&disconnected]() { return disconnected.load(); }, 5000);

  EXPECT_TRUE(closed_gracefully) << "Server did not detect disconnection via callback";

  // Check if server is still running/alive (didn't crash)
  // We can try to connect again to verify it's still accepting
  tcp::socket socket2(ioc);
  try {
    socket2.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), test_port_));
    EXPECT_TRUE(socket2.is_open());
  } catch (...) {
    FAIL() << "Server seems dead after RST";
  }

  // Explicitly stop server to prevent callbacks from accessing destroyed stack variables
  // when socket2 is destroyed (triggered by scope exit)
  server_->stop();
}
