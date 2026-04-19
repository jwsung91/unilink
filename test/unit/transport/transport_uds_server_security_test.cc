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

#ifndef _WIN32

#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <thread>

#include "test/utils/test_utils.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/transport/uds/uds_server.hpp"

using namespace unilink;
using namespace unilink::transport;
namespace net = boost::asio;
using uds_socket = net::local::stream_protocol::socket;
using uds_endpoint = net::local::stream_protocol::endpoint;

class TransportUdsServerSecurityTest : public ::testing::Test {
 protected:
  void SetUp() override {
    socket_path_ = test::TestUtils::makeUniqueUdsSocketPath("uds_sec");
    test::TestUtils::removeFileIfExists(socket_path_);
  }

  void TearDown() override {
    if (server_) {
      server_->stop();
      server_.reset();
    }
    test::TestUtils::removeFileIfExists(socket_path_);
  }

  std::shared_ptr<UdsServer> server_;
  std::filesystem::path socket_path_;
};

TEST_F(TransportUdsServerSecurityTest, NoIdleTimeoutByDefault) {
  config::UdsServerConfig cfg;
  cfg.socket_path = socket_path_.string();

  server_ = UdsServer::create(cfg);
  server_->start();

  ASSERT_TRUE(test::TestUtils::waitForCondition(
      [&] { return server_->get_state() == unilink::base::LinkState::Listening; }, 5000))
      << "Server failed to enter listening state";

  net::io_context client_ioc;
  uds_socket client(client_ioc);
  boost::system::error_code ec;

  for (int i = 0; i < 50; ++i) {
    client = uds_socket(client_ioc);
    client.connect(uds_endpoint(socket_path_.string()), ec);
    if (!ec) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  ASSERT_FALSE(ec) << "Failed to connect to UDS server";

  // Idle for 2 seconds — should remain connected with no timeout configured
  std::this_thread::sleep_for(std::chrono::seconds(2));

  net::write(client, net::buffer("ping"), ec);
  EXPECT_FALSE(ec) << "Client should still be connected (no idle timeout by default)";
}

TEST_F(TransportUdsServerSecurityTest, IdleConnectionTimeout) {
  config::UdsServerConfig cfg;
  cfg.socket_path = socket_path_.string();
  cfg.idle_timeout_ms = 1000;  // 1 second

  server_ = UdsServer::create(cfg);
  server_->start();

  ASSERT_TRUE(test::TestUtils::waitForCondition(
      [&] { return server_->get_state() == unilink::base::LinkState::Listening; }, 5000))
      << "Server failed to enter listening state";

  net::io_context client_ioc;
  uds_socket client(client_ioc);
  boost::system::error_code ec;

  for (int i = 0; i < 50; ++i) {
    client = uds_socket(client_ioc);
    client.connect(uds_endpoint(socket_path_.string()), ec);
    if (!ec) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  ASSERT_FALSE(ec) << "Failed to connect to UDS server";

  // Send within timeout — should succeed
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  net::write(client, net::buffer("ping"), ec);
  EXPECT_FALSE(ec) << "Client should still be connected (not timed out yet)";

  // Now idle past the timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  char buf[16];
  client.read_some(net::buffer(buf), ec);
  EXPECT_TRUE(ec == net::error::eof || ec == net::error::connection_reset || ec == net::error::broken_pipe)
      << "Client should have been disconnected due to idle timeout. Error: " << ec.message();
}

TEST_F(TransportUdsServerSecurityTest, IdleTimeoutResetOnMessage) {
  config::UdsServerConfig cfg;
  cfg.socket_path = socket_path_.string();
  cfg.idle_timeout_ms = 1000;  // 1 second

  server_ = UdsServer::create(cfg);
  server_->start();

  ASSERT_TRUE(test::TestUtils::waitForCondition(
      [&] { return server_->get_state() == unilink::base::LinkState::Listening; }, 5000))
      << "Server failed to enter listening state";

  net::io_context client_ioc;
  uds_socket client(client_ioc);
  boost::system::error_code ec;

  for (int i = 0; i < 50; ++i) {
    client = uds_socket(client_ioc);
    client.connect(uds_endpoint(socket_path_.string()), ec);
    if (!ec) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  ASSERT_FALSE(ec) << "Failed to connect to UDS server";

  // Send messages every 600ms for 3 seconds — each send resets the 1s timer
  for (int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    net::write(client, net::buffer("keepalive"), ec);
    ASSERT_FALSE(ec) << "Write " << i << " failed — client was disconnected prematurely";
  }
}

#endif  // _WIN32
