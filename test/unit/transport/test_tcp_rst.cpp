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
#include <thread>

#include "test_utils.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"

using namespace unilink::wrapper;
using namespace unilink::test;

namespace {

class TcpRstTest : public ::testing::Test {
 protected:
  void SetUp() override {
    port_ = TestUtils::getAvailableTestPort();
    server_ = std::make_shared<TcpServer>(port_);

    server_->on_multi_connect([this](size_t id, const std::string&) { connected_clients_++; });

    server_->on_multi_disconnect([this](size_t id) { disconnected_clients_++; });

    server_->start();
    ASSERT_TRUE(TestUtils::waitForCondition([this] { return server_->is_listening(); }, 2000));
  }

  void TearDown() override {
    if (server_) {
      // Explicitly clear callbacks to prevent them from firing after this fixture is destroyed
      server_->on_multi_connect(nullptr);
      server_->on_multi_disconnect(nullptr);

      server_->stop();
      // Ensure wrapper is destroyed before atomics are destroyed to prevent use-after-free in pending callbacks
      server_.reset();
    }
  }

  uint16_t port_;
  std::shared_ptr<TcpServer> server_;
  std::atomic<int> connected_clients_{0};
  std::atomic<int> disconnected_clients_{0};
};

TEST_F(TcpRstTest, ConnectionReset) {
  boost::asio::io_context ioc;
  boost::asio::ip::tcp::socket socket(ioc);

  // 1. Connect
  boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address("127.0.0.1"), port_);
  boost::system::error_code ec;
  socket.connect(ep, ec);
  ASSERT_FALSE(ec) << "Connect failed: " << ec.message();

  // Wait for server to register connection
  ASSERT_TRUE(TestUtils::waitForCondition([this] { return connected_clients_ > 0; }));

  // 2. Set SO_LINGER to 0 to force RST on close
  boost::asio::socket_base::linger option(true, 0);
  socket.set_option(option, ec);
  ASSERT_FALSE(ec);

  // 3. Close immediately
  socket.close(ec);
  ASSERT_FALSE(ec);

  // 4. Verify server handles it (should disconnect)
  // The session error handler should catch connection_reset and close session
  ASSERT_TRUE(TestUtils::waitForCondition([this] { return disconnected_clients_ > 0; }));
}

}  // namespace
