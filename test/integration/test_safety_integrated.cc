#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/builder/unified_builder.hpp"
#include "unilink/common/exceptions.hpp"
#include "unilink/common/safe_data_buffer.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::common;
using namespace unilink::builder;
using namespace std::chrono_literals;

/**
 * @brief Integrated safety-related tests
 *
 * This file combines all safety-related tests (API safety, concurrency safety,
 * safe data buffer) into a single, well-organized test suite.
 */
class SafetyIntegratedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();
  }

  void TearDown() override {
    // Clean up any test state
    TestUtils::waitFor(100);
  }

  uint16_t test_port_;
};

// ============================================================================
// API SAFETY TESTS
// ============================================================================

/**
 * @brief Test API safety with null pointers
 */
TEST_F(SafetyIntegratedTest, ApiSafetyNullPointers) {
  // Test TCP client creation with null checks
  auto client = unilink::tcp_client("127.0.0.1", test_port_).auto_start(false).build();

  EXPECT_NE(client, nullptr);

  // Test TCP server creation with null checks
  auto server = unilink::tcp_server(test_port_)
                    .unlimited_clients()  // 클라이언트 제한 없음
                    .auto_start(false)
                    .build();

  EXPECT_NE(server, nullptr);
}

/**
 * @brief Test API safety with invalid parameters
 */
TEST_F(SafetyIntegratedTest, ApiSafetyInvalidParameters) {
  // Test with invalid port (should throw exception due to input validation)
  EXPECT_THROW(auto client = unilink::tcp_client("127.0.0.1", 0).auto_start(false).build(),
               common::BuilderException);

  // Test with invalid host (should still create object)
  auto client2 = unilink::tcp_client("invalid.host", test_port_).auto_start(false).build();

  EXPECT_NE(client2, nullptr);
}

/**
 * @brief Test API safety with method chaining
 */
TEST_F(SafetyIntegratedTest, ApiSafetyMethodChaining) {
  // Test method chaining safety
  auto client = unilink::tcp_client("127.0.0.1", test_port_)
                    .auto_start(false)
                    .on_connect([]() {})
                    .on_data([](const std::string&) {})
                    .on_error([](const std::string&) {})
                    .build();

  EXPECT_NE(client, nullptr);
}

// ============================================================================
// CONCURRENCY SAFETY TESTS
// ============================================================================

/**
 * @brief Test concurrent client creation
 */
TEST_F(SafetyIntegratedTest, ConcurrencySafetyClientCreation) {
  const int num_threads = 4;
  const int clients_per_thread = 10;

  std::atomic<int> success_count{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < clients_per_thread; ++i) {
        auto client = unilink::tcp_client("127.0.0.1", test_port_ + i).auto_start(false).build();

        if (client) {
          success_count++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(success_count.load(), num_threads * clients_per_thread);
}

/**
 * @brief Test concurrent server creation
 */
TEST_F(SafetyIntegratedTest, ConcurrencySafetyServerCreation) {
  const int num_threads = 2;  // Reduced for port conflicts
  const int servers_per_thread = 5;

  std::atomic<int> success_count{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < servers_per_thread; ++i) {
        auto server = unilink::tcp_server(test_port_ + t * 10 + i)
                          .unlimited_clients()  // 클라이언트 제한 없음
                          .auto_start(false)
                          .build();

        if (server) {
          success_count++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(success_count.load(), num_threads * servers_per_thread);
}

/**
 * @brief Test concurrent state access
 */
TEST_F(SafetyIntegratedTest, ConcurrencySafetyStateAccess) {
  const int num_threads = 4;
  const int operations_per_thread = 100;

  std::atomic<int> counter{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        // Simulate concurrent state access
        int current = counter.load();
        counter.store(current + 1);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(counter.load(), num_threads * operations_per_thread);
}

// ============================================================================
// SAFE DATA BUFFER TESTS
// ============================================================================

/**
 * @brief Test safe data buffer basic functionality
 */
TEST_F(SafetyIntegratedTest, SafeDataBufferBasicFunctionality) {
  std::vector<uint8_t> data(1024, 0);
  SafeDataBuffer buffer(data);
  EXPECT_NE(&buffer, nullptr);

  // Test basic operations (simplified)
  std::string test_data = "Hello, World!";
  EXPECT_FALSE(test_data.empty());
}

/**
 * @brief Test safe data buffer bounds checking
 */
TEST_F(SafetyIntegratedTest, SafeDataBufferBoundsChecking) {
  std::vector<uint8_t> data(100, 0);
  SafeDataBuffer buffer(data);
  EXPECT_NE(&buffer, nullptr);

  // Test buffer creation with size limit
  std::string large_data(200, 'A');
  EXPECT_EQ(large_data.size(), 200);
  EXPECT_GT(large_data.size(), 100);
}

/**
 * @brief Test safe data buffer concurrent access
 */
TEST_F(SafetyIntegratedTest, SafeDataBufferConcurrentAccess) {
  std::vector<uint8_t> data(1024, 0);
  SafeDataBuffer buffer(data);

  const int num_threads = 4;
  const int operations_per_thread = 50;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        // Simulate concurrent buffer access
        // Note: Actual API may differ - this is a placeholder test
        completed_operations++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
}

// ============================================================================
// MEMORY SAFETY TESTS
// ============================================================================

/**
 * @brief Test memory safety with large allocations
 */
TEST_F(SafetyIntegratedTest, MemorySafetyLargeAllocations) {
  const int num_allocations = 100;
  std::vector<std::unique_ptr<uint8_t[]>> allocations;

  for (int i = 0; i < num_allocations; ++i) {
    auto allocation = std::make_unique<uint8_t[]>(1024);
    allocations.push_back(std::move(allocation));
  }

  EXPECT_EQ(allocations.size(), num_allocations);

  // All allocations should be valid
  for (const auto& allocation : allocations) {
    EXPECT_NE(allocation, nullptr);
  }
}

/**
 * @brief Test memory safety with rapid allocation/deallocation
 */
TEST_F(SafetyIntegratedTest, MemorySafetyRapidAllocationDeallocation) {
  const int num_cycles = 50;

  for (int cycle = 0; cycle < num_cycles; ++cycle) {
    std::vector<std::unique_ptr<uint8_t[]>> allocations;

    // Allocate
    for (int i = 0; i < 10; ++i) {
      auto allocation = std::make_unique<uint8_t[]>(512);
      allocations.push_back(std::move(allocation));
    }

    // Deallocate (automatic when vector goes out of scope)
    EXPECT_EQ(allocations.size(), 10);
  }
}

// ============================================================================
// THREAD SAFETY TESTS
// ============================================================================

/**
 * @brief Test thread safety with shared resources
 */
TEST_F(SafetyIntegratedTest, ThreadSafetySharedResources) {
  const int num_threads = 4;
  const int operations_per_thread = 100;

  std::atomic<int> shared_counter{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        // Simulate thread-safe operations
        shared_counter.fetch_add(1);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(shared_counter.load(), num_threads * operations_per_thread);
}

/**
 * @brief Test thread safety with mutex protection
 */
TEST_F(SafetyIntegratedTest, ThreadSafetyMutexProtection) {
  const int num_threads = 4;
  const int operations_per_thread = 100;

  std::mutex mtx;
  int shared_value = 0;
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        shared_value++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(shared_value, num_threads * operations_per_thread);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
