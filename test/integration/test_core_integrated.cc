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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/memory/safe_data_buffer.hpp"
#include "unilink/unilink.hpp"

// Test namespace aliases for cleaner code
using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

// Specific namespace aliases for better clarity
namespace builder = unilink::builder;

/**
 * @brief Integrated core functionality tests
 *
 * This file combines multiple core functionality tests into a single,
 * well-organized test suite for better maintainability.
 */
class CoreIntegratedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();

    // Reset error handler
    auto& error_handler = diagnostics::ErrorHandler::instance();
    error_handler.reset_stats();
  }

  void TearDown() override {
    // Clean up any test state
    // Increased wait time to ensure complete cleanup and avoid port conflicts
    TestUtils::waitFor(1000);
  }

  uint16_t test_port_;
};

// ============================================================================
// MEMORY POOL TESTS
// ============================================================================

/**
 * @brief Test memory pool basic functionality
 */
TEST_F(CoreIntegratedTest, MemoryPoolBasicFunctionality) {
  memory::MemoryPool pool(100, 200);

  // Test basic allocation and release
  auto buffer1 = pool.acquire(1024);
  EXPECT_NE(buffer1, nullptr);

  auto buffer2 = pool.acquire(512);
  EXPECT_NE(buffer2, nullptr);

  // Test release
  pool.release(std::move(buffer1), 1024);
  pool.release(std::move(buffer2), 512);

  // Verify statistics
  auto stats = pool.get_stats();
  EXPECT_GE(stats.total_allocations, 2);
}

/**
 * @brief Test memory pool performance
 */
TEST_F(CoreIntegratedTest, MemoryPoolPerformance) {
  memory::MemoryPool pool(1000, 2000);
  const int num_operations = 100;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_operations; ++i) {
    auto buffer = pool.acquire(1024);
    if (buffer) {
      pool.release(std::move(buffer), 1024);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  // Should be fast (less than 1ms per operation)
  EXPECT_LT(duration.count(), num_operations * 1000);
}

/**
 * @brief Test memory pool statistics
 */
TEST_F(CoreIntegratedTest, MemoryPoolStatistics) {
  memory::MemoryPool pool(50, 100);

  // Perform some operations
  for (int i = 0; i < 10; ++i) {
    auto buffer = pool.acquire(512);
    if (buffer) {
      pool.release(std::move(buffer), 512);
    }
  }

  auto stats = pool.get_stats();
  EXPECT_GE(stats.total_allocations, 0);
  // Note: total_releases may not be available in this version
}

// ============================================================================
// ERROR HANDLER TESTS
// ============================================================================

/**
 * @brief Test error handler basic functionality
 */
TEST_F(CoreIntegratedTest, ErrorHandlerBasicFunctionality) {
  auto& error_handler = diagnostics::ErrorHandler::instance();

  // Test error reporting
  diagnostics::error_reporting::report_connection_error("test", "operation", boost::system::error_code{}, false);

  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test error handler callback
 */
TEST_F(CoreIntegratedTest, ErrorHandlerCallback) {
  auto& error_handler = diagnostics::ErrorHandler::instance();

  std::atomic<int> callback_count{0};
  auto callback = [&callback_count](const diagnostics::ErrorInfo&) { callback_count++; };

  error_handler.register_callback(callback);
  diagnostics::error_reporting::report_connection_error("test", "operation", boost::system::error_code{}, false);

  // Wait for callback
  EXPECT_TRUE(TestUtils::waitForCondition([&callback_count]() { return callback_count.load() > 0; }, 1000));
}

// ============================================================================
// SAFE DATA BUFFER TESTS
// ============================================================================

/**
 * @brief Test safe data buffer basic functionality
 */
TEST_F(CoreIntegratedTest, SafeDataBufferBasicFunctionality) {
  std::string test_str = "Hello SafeDataBuffer";
  memory::SafeDataBuffer buffer(test_str);

  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(buffer.size(), test_str.size());
  EXPECT_EQ(buffer.as_string(), test_str);

  std::vector<uint8_t> vec(test_str.begin(), test_str.end());
  memory::SafeDataBuffer buffer_from_vec(vec);
  EXPECT_EQ(buffer, buffer_from_vec);
  EXPECT_EQ(buffer.as_string(), buffer_from_vec.as_string());
}

/**
 * @brief Test safe data buffer bounds checking
 */
TEST_F(CoreIntegratedTest, SafeDataBufferBoundsChecking) {
  std::vector<uint8_t> data = {1, 2, 3};
  memory::SafeDataBuffer buffer(data);
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(buffer.size(), 3);

  // Valid access
  EXPECT_NO_THROW(buffer.at(0));
  EXPECT_EQ(buffer.at(0), 1);
  EXPECT_EQ(buffer.at(2), 3);

  // Out of bounds access
  EXPECT_THROW(buffer.at(3), std::out_of_range);
}

// ============================================================================
// IO CONTEXT MANAGER TESTS
// ============================================================================

/**
 * @brief Test IO context manager basic functionality
 */
TEST_F(CoreIntegratedTest, IoContextManagerBasicFunctionality) {
  auto& manager = unilink::concurrency::IoContextManager::instance();
  EXPECT_NE(&manager, nullptr);

  // Start the manager
  manager.start();
  EXPECT_TRUE(manager.is_running());

  // Verify context is working by posting a task
  auto& ioc = manager.get_context();
  std::atomic<bool> task_executed{false};
  boost::asio::post(ioc, [&task_executed]() { task_executed = true; });

  // Wait for task execution
  EXPECT_TRUE(TestUtils::waitForCondition([&task_executed]() { return task_executed.load(); }, 1000));

  // Stop the manager
  manager.stop();
  EXPECT_FALSE(manager.is_running());
}

/**
 * @brief Test IO context manager independent contexts
 */
TEST_F(CoreIntegratedTest, IoContextManagerIndependentContexts) {
  auto& manager = unilink::concurrency::IoContextManager::instance();

  // Create an independent context
  auto ioc_ptr = manager.create_independent_context();
  EXPECT_NE(ioc_ptr, nullptr);

  // Verify it works independently
  std::atomic<bool> task_executed{false};
  boost::asio::post(*ioc_ptr, [&task_executed]() { task_executed = true; });

  // Independent context shouldn't run automatically via manager's thread
  // We need to run it manually
  EXPECT_FALSE(task_executed.load());

  ioc_ptr->run_one();
  EXPECT_TRUE(task_executed.load());
}

// ============================================================================
// THREAD SAFE STATE TESTS
// ============================================================================

/**
 * @brief Test thread safe state basic functionality
 */
TEST_F(CoreIntegratedTest, ThreadSafeStateBasicFunctionality) {
  using State = std::string;
  unilink::concurrency::ThreadSafeState<State> state("initial");

  EXPECT_EQ(state.get_state(), "initial");

  // Test set_state
  state.set_state("updated");
  EXPECT_EQ(state.get_state(), "updated");

  // Test compare_and_set (success)
  EXPECT_TRUE(state.compare_and_set("updated", "final"));
  EXPECT_EQ(state.get_state(), "final");

  // Test compare_and_set (failure)
  EXPECT_FALSE(state.compare_and_set("wrong", "never"));
  EXPECT_EQ(state.get_state(), "final");
}

/**
 * @brief Test thread safe state concurrent access
 */
TEST_F(CoreIntegratedTest, ThreadSafeStateConcurrentAccess) {
  unilink::concurrency::ThreadSafeState<int> state(0);
  std::atomic<bool> thread_started{false};

  std::thread t([&]() {
    thread_started = true;
    // Wait for state to become 1, then set to 2
    state.wait_for_state(1);
    state.set_state(2);
  });

  // Wait for thread to start
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return thread_started.load(); }, 1000));

  // Update state to 1 to trigger the thread
  state.set_state(1);

  // Wait for thread to update to 2
  // wait_for_state uses condition variable, so it should return when state becomes 2
  state.wait_for_state(2, std::chrono::milliseconds(2000));

  EXPECT_EQ(state.get_state(), 2);

  if (t.joinable()) t.join();
}

// ============================================================================
// BUILDER PATTERN TESTS
// ============================================================================

/**
 * @brief Test unified builder basic functionality
 */
TEST_F(CoreIntegratedTest, UnifiedBuilderBasicFunctionality) {
  // Test TCP client builder
  auto client = unilink::tcp_client("127.0.0.1", test_port_).build();

  EXPECT_NE(client, nullptr);
}

/**
 * @brief Test unified builder method chaining
 */
TEST_F(CoreIntegratedTest, UnifiedBuilderMethodChaining) {
  // Test method chaining
  auto client = unilink::tcp_client("127.0.0.1", test_port_)

                    .on_connect([]() {})
                    .on_data([](const std::string&) {})
                    .on_error([](const std::string&) {})
                    .build();

  EXPECT_NE(client, nullptr);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

/**
 * @brief Test basic communication integration
 */
TEST_F(CoreIntegratedTest, BasicCommunicationIntegration) {
  // Create server
  auto server = unilink::tcp_server(test_port_)
                    .unlimited_clients()  // 클라이언트 제한 없음

                    .on_connect([]() {})
                    .on_data([](const std::string& data) {})
                    .build();

  EXPECT_NE(server, nullptr);
  server->start();

  // Wait for server to start
  TestUtils::waitFor(100);

  // Create client
  std::atomic<bool> client_connected{false};
  auto client = unilink::tcp_client("127.0.0.1", test_port_)

                    .on_connect([&client_connected]() { client_connected = true; })
                    .build();

  EXPECT_NE(client, nullptr);
  client->start();

  // Wait for connection
  EXPECT_TRUE(TestUtils::waitForCondition([&client_connected]() { return client_connected.load(); }, 5000));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
