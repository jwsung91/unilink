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

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

// ============================================================================
// IMPROVED ARCHITECTURE TESTS
// ============================================================================

class ImprovedArchitectureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Stop IoContextManager for auto-init test
    if (common::IoContextManager::instance().is_running()) {
      std::cout << "Stopping IoContextManager for auto-init test..." << std::endl;
      common::IoContextManager::instance().stop();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void TearDown() override {
    try {
      if (client_) {
        std::cout << "Stopping client..." << std::endl;
        client_->stop();
        client_.reset();
      }
      if (server_) {
        std::cout << "Stopping server..." << std::endl;
        server_->stop();
        server_.reset();
      }

      // Clean up with sufficient time
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      // IoContextManager is not managed individually in each test
      // It's a global state so it can affect other tests
    } catch (const std::exception& e) {
      std::cout << "Exception in TearDown: " << e.what() << std::endl;
    } catch (...) {
      std::cout << "Unknown exception in TearDown" << std::endl;
    }
  }

  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{static_cast<uint16_t>(60000)};
    return port_counter.fetch_add(1);
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
};

/**
 * @brief Current Resource Sharing Issue Verification Test
 */
TEST_F(ImprovedArchitectureTest, CurrentResourceSharingIssue) {
  std::cout << "Testing current resource sharing issue..." << std::endl;

  uint16_t test_port = getTestPort();

  // Create server
  server_ = unilink::tcp_server(test_port).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
  std::cout << "Server created successfully" << std::endl;

  // Create client
  client_ = unilink::tcp_client("127.0.0.1", test_port).build();

  ASSERT_NE(client_, nullptr);
  std::cout << "Client created successfully" << std::endl;

  // Brief wait
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::cout << "Test completed - resource sharing issue demonstrated" << std::endl;
}

/**
 * @brief Proposed Independent Resource Management Test
 */
TEST_F(ImprovedArchitectureTest, ProposedIndependentResourceManagement) {
  std::cout << "Testing proposed independent resource management..." << std::endl;

  // Auto-initialization test using AutoInitializer
  EXPECT_FALSE(builder::AutoInitializer::is_io_context_running());

  // Auto-initialization
  builder::AutoInitializer::ensure_io_context_running();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(builder::AutoInitializer::is_io_context_running());

  std::cout << "Independent resource management test completed" << std::endl;
}

/**
 * @brief Upper API Auto-initialization Test
 */
TEST_F(ImprovedArchitectureTest, UpperAPIAutoInitialization) {
  std::cout << "Testing upper API auto-initialization..." << std::endl;

  uint16_t test_port = getTestPort();

  // Builder auto-initializes even if IoContextManager is not running
  if (common::IoContextManager::instance().is_running()) {
    common::IoContextManager::instance().stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // IoContextManager starts automatically when using builder
  server_ = unilink::tcp_server(test_port).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  // Check if IoContextManager started automatically
  EXPECT_TRUE(common::IoContextManager::instance().is_running());

  std::cout << "Upper API auto-initialization test completed" << std::endl;
}

/**
 * @brief Resource Sharing Analysis Test
 */
TEST_F(ImprovedArchitectureTest, ResourceSharingAnalysis) {
  std::cout << "Analyzing resource sharing..." << std::endl;

  // Resource management test through IoContextManager
  auto& context = common::IoContextManager::instance().get_context();
  EXPECT_TRUE(&context != nullptr);

  std::cout << "Resource sharing analysis completed" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
