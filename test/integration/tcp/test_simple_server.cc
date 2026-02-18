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

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "test_constants.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

class SimpleServerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize before test
  }

  void TearDown() override {
    if (server_) {
      std::cout << "Stopping server..." << std::endl;
      server_->stop();
    }
    std::this_thread::sleep_for(test::constants::kShortTimeout);
  }

  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{50000};
    return port_counter.fetch_add(1);
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
};

/**
 * @brief Simplest Server Creation Test
 */
TEST_F(SimpleServerTest, BasicServerCreation) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing basic server creation with port: " << test_port << std::endl;

  // Create server
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()  // No client limit
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created successfully" << std::endl;

  // Start server and wait for result
  std::cout << "Starting server..." << std::endl;
  EXPECT_TRUE(server_->start().get());

  std::cout << "Server state: " << (server_->is_listening() ? "listening" : "not listening") << std::endl;

  // Check if server was created
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Server Auto-Start Test
 */
TEST_F(SimpleServerTest, AutoStartServer) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing auto-start server with port: " << test_port << std::endl;

  // Create server (auto-manage triggers start)
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .auto_manage(true)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created with auto-start" << std::endl;

  // Brief wait for auto-manage to kick in
  std::this_thread::sleep_for(test::constants::kMediumTimeout);

  std::cout << "Server state: " << (server_->is_listening() ? "listening" : "not listening") << std::endl;

  EXPECT_TRUE(server_->is_listening());
}

/**
 * @brief Server Callback Test
 */
TEST_F(SimpleServerTest, ServerWithCallbacks) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing server with callbacks, port: " << test_port << std::endl;

  auto connect_called = std::make_shared<std::atomic<bool>>(false);
  auto error_called = std::make_shared<std::atomic<bool>>(false);
  auto last_error = std::make_shared<std::string>();

  // Create server
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .on_connect([connect_called](const wrapper::ConnectionContext& ctx) {
                  std::cout << "Connect callback called for client: " << ctx.client_id() << std::endl;
                  connect_called->store(true);
                })
                .on_error([error_called, last_error](const wrapper::ErrorContext& ctx) {
                  std::cout << "Error callback called: " << ctx.message() << std::endl;
                  error_called->store(true);
                  *last_error = std::string(ctx.message());
                })
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_TRUE(server_->start().get());

  std::cout << "Server is now listening" << std::endl;
}

/**
 * @brief Server Status Verification Test
 */
TEST_F(SimpleServerTest, ServerStateCheck) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing server state check, port: " << test_port << std::endl;

  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .build();

  ASSERT_NE(server_, nullptr);

  // Status before start
  EXPECT_FALSE(server_->is_listening()) << "Server should not be listening before start";

  // Start server
  EXPECT_TRUE(server_->start().get());
  EXPECT_TRUE(server_->is_listening());

  server_->stop();
  EXPECT_FALSE(server_->is_listening());
}

/**
 * @brief Client Limit Feature Test - Single Client
 */
TEST_F(SimpleServerTest, ClientLimitSingleClient) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port)
                .single_client()
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_TRUE(server_->start().get());
  EXPECT_TRUE(server_->is_listening());
}

/**
 * @brief Client Limit Feature Test - Multi Client
 */
TEST_F(SimpleServerTest, ClientLimitMultiClient) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port)
                .multi_client(3)
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_TRUE(server_->start().get());
}

/**
 * @brief Client Limit Feature Test - Unlimited Clients
 */
TEST_F(SimpleServerTest, ClientLimitUnlimitedClients) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_TRUE(server_->start().get());
}

/**
 * @brief Client Limit Feature Test - Builder Validation
 */
TEST_F(SimpleServerTest, ClientLimitBuilderValidation) {
  uint16_t test_port = getTestPort();
  EXPECT_THROW(
      {
        server_ = unilink::tcp_server(test_port)
                      .multi_client(0)
                      .build();
      },
      std::invalid_argument);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
