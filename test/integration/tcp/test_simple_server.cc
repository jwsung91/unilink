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

  // Start server
  std::cout << "Starting server..." << std::endl;
  server_->start();

  // Brief wait
  std::this_thread::sleep_for(test::constants::kDefaultTimeout);

  std::cout << "Server state: " << (server_->is_connected() ? "connected" : "not connected") << std::endl;

  // Check if server was created
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Server Auto-Start Test
 */
TEST_F(SimpleServerTest, AutoStartServer) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing auto-start server with port: " << test_port << std::endl;

  // Create server (자동 시작)
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()  // No client limit

                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created with auto-start" << std::endl;

  // Brief wait
  std::this_thread::sleep_for(test::constants::kMediumTimeout);

  std::cout << "Server state after 2s: " << (server_->is_connected() ? "connected" : "not connected") << std::endl;

  // Check if server was created
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Server Callback Test
 */
TEST_F(SimpleServerTest, ServerWithCallbacks) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing server with callbacks, port: " << test_port << std::endl;

  // Use shared_ptr to ensure variables live long enough
  auto connect_called = std::make_shared<std::atomic<bool>>(false);
  auto error_called = std::make_shared<std::atomic<bool>>(false);
  auto last_error = std::make_shared<std::string>();

  // Create server (콜백 포함)
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()  // No client limit

                .on_connect([connect_called]() {
                  std::cout << "Connect callback called!" << std::endl;
                  connect_called->store(true);
                })
                .on_error([error_called, last_error](const std::string& error) {
                  std::cout << "Error callback called: " << error << std::endl;
                  error_called->store(true);
                  *last_error = error;
                })
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created with callbacks" << std::endl;

  // Brief wait
  std::this_thread::sleep_for(3000ms);

  std::cout << "Server state after 3s: " << (server_->is_connected() ? "connected" : "not connected") << std::endl;
  std::cout << "Connect callback called: " << (connect_called->load() ? "yes" : "no") << std::endl;
  std::cout << "Error callback called: " << (error_called->load() ? "yes" : "no") << std::endl;
  if (error_called->load()) {
    std::cout << "Last error: " << *last_error << std::endl;
  }

  // Check if server was created
  EXPECT_TRUE(server_ != nullptr);

  // Print if error occurred
  if (error_called->load()) {
    std::cout << "Server encountered error: " << *last_error << std::endl;
  }
}

/**
 * @brief Server Status Verification Test
 */
TEST_F(SimpleServerTest, ServerStateCheck) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing server state check, port: " << test_port << std::endl;

  // Create server
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()  // No client limit

                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";

  // Status before start
  std::cout << "Before start - is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;
  EXPECT_FALSE(server_->is_connected()) << "Server should not be connected before start";

  // Start server
  std::cout << "Starting server..." << std::endl;
  server_->start();

  // Check status after start (multiple times)
  for (int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for(test::constants::kDefaultTimeout);
    std::cout << "After " << (i + 1) << "s - is_connected(): " << (server_->is_connected() ? "true" : "false")
              << std::endl;
  }

  // Check if server was created
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Client Limit Feature Test - Single Client
 */
TEST_F(SimpleServerTest, ClientLimitSingleClient) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing single client limit, port: " << test_port << std::endl;

  // Create single client server
  server_ = unilink::tcp_server(test_port)
                .single_client()  // Allow only 1 client

                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Single client server created" << std::endl;

  // Start server
  server_->start();
  std::this_thread::sleep_for(test::constants::kDefaultTimeout);

  std::cout << "Single client server started" << std::endl;
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Client Limit Feature Test - Multi Client
 */
TEST_F(SimpleServerTest, ClientLimitMultiClient) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing multi client limit (3 clients), port: " << test_port << std::endl;

  // Create multi client server (limit 3 clients)
  server_ = unilink::tcp_server(test_port)
                .multi_client(3)  // Allow only 3 clients

                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Multi client server (limit 3) created" << std::endl;

  // Start server
  server_->start();
  std::this_thread::sleep_for(test::constants::kDefaultTimeout);

  std::cout << "Multi client server started" << std::endl;
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Client Limit Feature Test - Unlimited Clients
 */
TEST_F(SimpleServerTest, ClientLimitUnlimitedClients) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing unlimited clients, port: " << test_port << std::endl;

  // Create unlimited clients server
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()  // No client limit

                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Unlimited clients server created" << std::endl;

  // Start server
  server_->start();
  std::this_thread::sleep_for(test::constants::kDefaultTimeout);

  std::cout << "Unlimited clients server started" << std::endl;
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief Client Limit Feature Test - Builder Validation
 */
TEST_F(SimpleServerTest, ClientLimitBuilderValidation) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing client limit builder validation, port: " << test_port << std::endl;

  // Attempt to create server with invalid settings (0 clients)
  EXPECT_THROW(
      {
        server_ = unilink::tcp_server(test_port)
                      .multi_client(0)  // 0 is invalid

                      .build();
      },
      std::invalid_argument)
      << "Should throw exception for 0 client limit";

  std::cout << "Builder validation test passed" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
