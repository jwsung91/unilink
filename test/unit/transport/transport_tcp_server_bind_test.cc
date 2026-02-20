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

#include <boost/asio.hpp>
#include <memory>
#include <thread>

#include "test_constants.hpp"
#include "test_utils.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TransportTcpServerBindTest : public ::testing::Test {
 protected:
  void TearDown() override {
    if (server_) {
      server_->stop();
      server_.reset();
    }
    // Give some time for io_context to cleanup
    TestUtils::waitFor(constants::kShortTimeout.count());
  }

  std::shared_ptr<TcpServer> server_;
};

TEST_F(TransportTcpServerBindTest, DefaultBindAddress) {
  config::TcpServerConfig cfg;
  cfg.port = TestUtils::getAvailableTestPort();
  // Default bind_address is "0.0.0.0"

  server_ = TcpServer::create(cfg);
  server_->start();

  // Wait a bit to ensure it enters listening state
  EXPECT_TRUE(TestUtils::waitForCondition([&] { return server_->get_state() == base::LinkState::Listening; },
                                          constants::kDefaultTimeout.count()));

  // Verify we can connect via loopback
  net::io_context client_ioc;
  tcp::socket client(client_ioc);
  boost::system::error_code ec;
  client.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), cfg.port), ec);

  EXPECT_FALSE(ec) << "Failed to connect to server bound to default (0.0.0.0) via 127.0.0.1: " << ec.message();
}

TEST_F(TransportTcpServerBindTest, LocalhostBindAddress) {
  config::TcpServerConfig cfg;
  cfg.port = TestUtils::getAvailableTestPort();
  cfg.bind_address = "127.0.0.1";

  server_ = TcpServer::create(cfg);
  server_->start();

  // Wait a bit to ensure it enters listening state
  EXPECT_TRUE(TestUtils::waitForCondition([&] { return server_->get_state() == base::LinkState::Listening; },
                                          constants::kDefaultTimeout.count()));

  // Verify we can connect via loopback
  net::io_context client_ioc;
  tcp::socket client(client_ioc);
  boost::system::error_code ec;
  client.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), cfg.port), ec);

  EXPECT_FALSE(ec) << "Failed to connect to server bound to 127.0.0.1: " << ec.message();
}

TEST_F(TransportTcpServerBindTest, InvalidBindAddress) {
  config::TcpServerConfig cfg;
  cfg.port = TestUtils::getAvailableTestPort();
  cfg.bind_address = "invalid.ip.address";

  server_ = TcpServer::create(cfg);

  std::atomic<bool> error_occurred{false};
  server_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Error) {
      error_occurred = true;
    }
  });

  server_->start();

  // Wait for error state
  EXPECT_TRUE(TestUtils::waitForCondition(
      [&] { return error_occurred.load() || server_->get_state() == base::LinkState::Error; },
      constants::kDefaultTimeout.count()));

  EXPECT_EQ(server_->get_state(), base::LinkState::Error);
}
