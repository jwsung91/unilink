#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/builder/unified_builder.hpp"
#include "unilink/common/memory_pool.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::common;
using namespace unilink::builder;
using namespace std::chrono_literals;

/**
 * @brief Comprehensive performance tests
 *
 * This file combines all performance-related tests including
 * basic performance, advanced performance, scalability, throughput,
 * latency, resource usage, and optimization testing.
 */
class PerformanceIntegratedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();

    // Reset memory pool for clean testing
    auto& pool = GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));

    // Initialize performance metrics
    start_time_ = std::chrono::high_resolution_clock::now();
  }

  void TearDown() override {
    // Clean up memory pool
    auto& pool = GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));

    // Calculate and log performance metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

    std::cout << "Test completed in " << duration.count() << " μs" << std::endl;

    // Clean up any test state
    TestUtils::waitFor(100);
  }

  uint16_t test_port_;
  std::chrono::high_resolution_clock::time_point start_time_;

  // Helper function to generate test data
  std::string generate_test_data(size_t size) {
    std::string data;
    data.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      data += static_cast<char>('A' + (i % 26));
    }
    return data;
  }

  // Helper function to measure throughput
  double measure_throughput(std::function<void()> operation, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
      operation();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    return static_cast<double>(iterations) / (duration.count() / 1000000.0);
  }

  // Helper function to measure latency
  double measure_latency(std::function<void()> operation, int iterations) {
    std::vector<double> latencies;
    latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
      auto start = std::chrono::high_resolution_clock::now();
      operation();
      auto end = std::chrono::high_resolution_clock::now();

      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
      latencies.push_back(duration.count() / 1000.0);  // Convert to microseconds
    }

    // Calculate median latency
    std::sort(latencies.begin(), latencies.end());
    return latencies[latencies.size() / 2];
  }
};

// ============================================================================
// BASIC PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Test transport performance benchmark
 */
TEST_F(PerformanceIntegratedTest, TransportPerformanceBenchmark) {
  std::cout << "\n=== Transport Performance Benchmark Test ===" << std::endl;

  const int num_operations = 1000;
  const size_t data_size = 1024;

  auto start_time = std::chrono::high_resolution_clock::now();

  // Simulate transport operations
  for (int i = 0; i < num_operations; ++i) {
    // Simulate data processing
    std::string data = generate_test_data(data_size);
    EXPECT_EQ(data.size(), data_size);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

  std::cout << "Transport performance: " << duration << " μs for " << num_operations << " operations" << std::endl;

  // Verify performance is reasonable
  EXPECT_LT(duration, 100000);  // Should complete in less than 100ms
}

/**
 * @brief Test concurrent performance
 */
TEST_F(PerformanceIntegratedTest, ConcurrentPerformanceTest) {
  std::cout << "\n=== Concurrent Performance Test ===" << std::endl;

  const int num_threads = 4;
  const int operations_per_thread = 1000;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        // Simulate work
        std::string data = generate_test_data(1024);
        completed_operations++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(completed_operations.load()) / (duration.count() / 1000000.0);

  std::cout << "Concurrent performance:" << std::endl;
  std::cout << "  Threads: " << num_threads << std::endl;
  std::cout << "  Operations: " << completed_operations.load() << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
  EXPECT_GT(throughput, 1000);
}

// ============================================================================
// MEMORY POOL PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Test memory pool performance under load
 */
TEST_F(PerformanceIntegratedTest, MemoryPoolPerformanceUnderLoad) {
  std::cout << "\n=== Memory Pool Performance Under Load Test ===" << std::endl;

  auto& pool = GlobalMemoryPool::instance();
  const int num_operations = 10000;
  const size_t buffer_size = 1024;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_operations; ++i) {
    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      pool.release(std::move(buffer), buffer_size);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_operations) / (duration.count() / 1000000.0);

  std::cout << "Memory pool performance:" << std::endl;
  std::cout << "  Operations: " << num_operations << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
  std::cout << "  Average per operation: " << (duration.count() / num_operations) << " μs" << std::endl;

  // Performance should be reasonable (at least 1000 ops/sec)
  EXPECT_GT(throughput, 1000);
}

/**
 * @brief Test memory pool throughput
 */
TEST_F(PerformanceIntegratedTest, MemoryPoolThroughput) {
  std::cout << "\n=== Memory Pool Throughput Test ===" << std::endl;

  auto& pool = GlobalMemoryPool::instance();
  const int num_operations = 10000;
  const size_t buffer_size = 1024;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_operations; ++i) {
    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      pool.release(std::move(buffer), buffer_size);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_operations) / (duration.count() / 1000000.0);

  std::cout << "Memory pool throughput: " << throughput << " ops/sec" << std::endl;
  std::cout << "Operations: " << num_operations << std::endl;
  std::cout << "Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "Average per operation: " << (duration.count() / num_operations) << " μs" << std::endl;

  // Throughput should be reasonable (at least 1000 ops/sec)
  EXPECT_GT(throughput, 1000);
}

// ============================================================================
// SCALABILITY TESTS
// ============================================================================

/**
 * @brief Test scalability with increasing thread count
 */
TEST_F(PerformanceIntegratedTest, ScalabilityThreadCount) {
  std::cout << "\n=== Scalability Thread Count Test ===" << std::endl;

  std::vector<int> thread_counts = {1, 2, 4, 8, 16, 32};
  const int operations_per_thread = 1000;

  for (auto thread_count : thread_counts) {
    std::atomic<int> completed_operations{0};
    std::vector<std::thread> threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int t = 0; t < thread_count; ++t) {
      threads.emplace_back([&]() {
        for (int i = 0; i < operations_per_thread; ++i) {
          // Simulate work
          std::string data = generate_test_data(1024);
          completed_operations++;
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    double throughput = static_cast<double>(completed_operations.load()) / (duration.count() / 1000000.0);

    std::cout << "Threads: " << thread_count << ", Operations: " << completed_operations.load()
              << ", Duration: " << duration.count() << " μs" << ", Throughput: " << throughput << " ops/sec"
              << std::endl;

    EXPECT_EQ(completed_operations.load(), thread_count * operations_per_thread);
  }
}

/**
 * @brief Test scalability with increasing data size
 */
TEST_F(PerformanceIntegratedTest, ScalabilityDataSize) {
  std::cout << "\n=== Scalability Data Size Test ===" << std::endl;

  std::vector<size_t> data_sizes = {1024, 4096, 16384, 65536, 262144, 1048576};
  const int operations = 100;

  for (auto data_size : data_sizes) {
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < operations; ++i) {
      std::string data = generate_test_data(data_size);
      EXPECT_EQ(data.size(), data_size);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    double throughput = static_cast<double>(operations) / (duration.count() / 1000000.0);
    double data_throughput = static_cast<double>(operations * data_size) / (duration.count() / 1000000.0);

    std::cout << "Data size: " << data_size << " bytes" << ", Operations: " << operations
              << ", Duration: " << duration.count() << " μs" << ", Throughput: " << throughput << " ops/sec"
              << ", Data throughput: " << data_throughput << " bytes/sec" << std::endl;
  }
}

// ============================================================================
// THROUGHPUT TESTS
// ============================================================================

/**
 * @brief Test network throughput simulation
 */
TEST_F(PerformanceIntegratedTest, NetworkThroughputSimulation) {
  std::cout << "\n=== Network Throughput Simulation Test ===" << std::endl;

  const int num_messages = 1000;
  const size_t message_size = 1024;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_messages; ++i) {
    std::string message = generate_test_data(message_size);
    // Simulate network processing
    EXPECT_EQ(message.size(), message_size);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_messages) / (duration.count() / 1000000.0);
  double data_throughput = static_cast<double>(num_messages * message_size) / (duration.count() / 1000000.0);

  std::cout << "Network throughput simulation:" << std::endl;
  std::cout << "  Messages: " << num_messages << std::endl;
  std::cout << "  Message size: " << message_size << " bytes" << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " messages/sec" << std::endl;
  std::cout << "  Data throughput: " << data_throughput << " bytes/sec" << std::endl;

  // Throughput should be reasonable
  EXPECT_GT(throughput, 100);
}

// ============================================================================
// LATENCY TESTS
// ============================================================================

/**
 * @brief Test operation latency
 */
TEST_F(PerformanceIntegratedTest, OperationLatency) {
  std::cout << "\n=== Operation Latency Test ===" << std::endl;

  const int num_operations = 1000;

  // Test memory allocation latency
  auto& pool = GlobalMemoryPool::instance();
  const size_t buffer_size = 1024;

  std::vector<double> latencies;
  latencies.reserve(num_operations);

  for (int i = 0; i < num_operations; ++i) {
    auto start = std::chrono::high_resolution_clock::now();

    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      pool.release(std::move(buffer), buffer_size);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    latencies.push_back(duration.count() / 1000.0);  // Convert to microseconds
  }

  // Calculate statistics
  std::sort(latencies.begin(), latencies.end());
  double min_latency = latencies[0];
  double max_latency = latencies[latencies.size() - 1];
  double median_latency = latencies[latencies.size() / 2];
  double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();

  std::cout << "Operation latency statistics:" << std::endl;
  std::cout << "  Min: " << min_latency << " μs" << std::endl;
  std::cout << "  Max: " << max_latency << " μs" << std::endl;
  std::cout << "  Median: " << median_latency << " μs" << std::endl;
  std::cout << "  Average: " << avg_latency << " μs" << std::endl;

  // Latency should be reasonable (less than 100μs)
  EXPECT_LT(median_latency, 100);
}

/**
 * @brief Test network latency simulation
 */
TEST_F(PerformanceIntegratedTest, NetworkLatencySimulation) {
  std::cout << "\n=== Network Latency Simulation Test ===" << std::endl;

  const int num_operations = 1000;
  const size_t data_size = 1024;

  std::vector<double> latencies;
  latencies.reserve(num_operations);

  for (int i = 0; i < num_operations; ++i) {
    auto start = std::chrono::high_resolution_clock::now();

    // Simulate network operation
    std::string data = generate_test_data(data_size);
    EXPECT_EQ(data.size(), data_size);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    latencies.push_back(duration.count() / 1000.0);  // Convert to microseconds
  }

  // Calculate statistics
  std::sort(latencies.begin(), latencies.end());
  double min_latency = latencies[0];
  double max_latency = latencies[latencies.size() - 1];
  double median_latency = latencies[latencies.size() / 2];
  double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();

  std::cout << "Network latency simulation statistics:" << std::endl;
  std::cout << "  Min: " << min_latency << " μs" << std::endl;
  std::cout << "  Max: " << max_latency << " μs" << std::endl;
  std::cout << "  Median: " << median_latency << " μs" << std::endl;
  std::cout << "  Average: " << avg_latency << " μs" << std::endl;

  // Latency should be reasonable (less than 50μs)
  EXPECT_LT(median_latency, 50);
}

// ============================================================================
// RESOURCE USAGE TESTS
// ============================================================================

/**
 * @brief Test memory usage under load
 */
TEST_F(PerformanceIntegratedTest, MemoryUsageUnderLoad) {
  std::cout << "\n=== Memory Usage Under Load Test ===" << std::endl;

  auto& pool = GlobalMemoryPool::instance();
  const int num_cycles = 100;
  const int buffers_per_cycle = 10;
  const size_t buffer_size = 1024;

  // Get initial memory stats
  auto initial_stats = pool.get_stats();
  size_t initial_allocations = initial_stats.total_allocations;

  std::cout << "Initial allocations: " << initial_allocations << std::endl;

  // Perform allocation cycles
  for (int cycle = 0; cycle < num_cycles; ++cycle) {
    std::vector<std::unique_ptr<uint8_t[]>> buffers;
    buffers.reserve(buffers_per_cycle);

    // Allocate buffers
    for (int i = 0; i < buffers_per_cycle; ++i) {
      auto buffer = pool.acquire(buffer_size);
      if (buffer) {
        buffers.push_back(std::move(buffer));
      }
    }

    // Release buffers
    for (auto& buffer : buffers) {
      pool.release(std::move(buffer), buffer_size);
    }

    // Periodic cleanup
    if (cycle % 20 == 0) {
      pool.cleanup_old_buffers(std::chrono::milliseconds(0));
    }
  }

  // Force cleanup
  pool.cleanup_old_buffers(std::chrono::milliseconds(0));

  // Get final memory stats
  auto final_stats = pool.get_stats();
  size_t final_allocations = final_stats.total_allocations;

  std::cout << "Final allocations: " << final_allocations << std::endl;
  std::cout << "Allocation difference: " << (final_allocations - initial_allocations) << std::endl;
  std::cout << "Total cycles: " << num_cycles << std::endl;
  std::cout << "Buffers per cycle: " << buffers_per_cycle << std::endl;

  // Memory usage should not grow excessively
  // Note: Memory pool may track all allocations, so we check for reasonable growth
  EXPECT_LE(final_allocations - initial_allocations, num_cycles * buffers_per_cycle * 2);
}

/**
 * @brief Test CPU usage under load
 */
TEST_F(PerformanceIntegratedTest, CPUUsageUnderLoad) {
  std::cout << "\n=== CPU Usage Under Load Test ===" << std::endl;

  const int num_threads = 4;
  const int operations_per_thread = 1000;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        // Simulate CPU-intensive work
        std::string data = generate_test_data(1024);
        std::sort(data.begin(), data.end());
        completed_operations++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(completed_operations.load()) / (duration.count() / 1000000.0);

  std::cout << "CPU usage under load:" << std::endl;
  std::cout << "  Threads: " << num_threads << std::endl;
  std::cout << "  Operations per thread: " << operations_per_thread << std::endl;
  std::cout << "  Total operations: " << completed_operations.load() << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
}

// ============================================================================
// OPTIMIZATION TESTS
// ============================================================================

/**
 * @brief Test lock-free performance
 */
TEST_F(PerformanceIntegratedTest, LockFreePerformance) {
  std::cout << "\n=== Lock-Free Performance Test ===" << std::endl;

  const int num_operations = 10000;
  std::atomic<int> counter{0};

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_operations; ++i) {
    counter.fetch_add(1);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_operations) / (duration.count() / 1000000.0);

  std::cout << "Lock-free performance:" << std::endl;
  std::cout << "  Operations: " << num_operations << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
  std::cout << "  Final counter: " << counter.load() << std::endl;

  EXPECT_EQ(counter.load(), num_operations);
  EXPECT_GT(throughput, 1000);
}

/**
 * @brief Test cache-friendly performance
 */
TEST_F(PerformanceIntegratedTest, CacheFriendlyPerformance) {
  std::cout << "\n=== Cache-Friendly Performance Test ===" << std::endl;

  const int num_elements = 1000000;
  std::vector<int> data(num_elements);

  // Initialize data
  for (int i = 0; i < num_elements; ++i) {
    data[i] = i;
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  // Sequential access (cache-friendly)
  int sum = 0;
  for (int i = 0; i < num_elements; ++i) {
    sum += data[i];
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_elements) / (duration.count() / 1000000.0);

  std::cout << "Cache-friendly performance:" << std::endl;
  std::cout << "  Elements: " << num_elements << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " elements/sec" << std::endl;
  std::cout << "  Sum: " << sum << std::endl;

  EXPECT_GT(throughput, 1000000);
}

// ============================================================================
// STRESS TESTS
// ============================================================================

/**
 * @brief Test system stress with high load
 */
TEST_F(PerformanceIntegratedTest, SystemStressHighLoad) {
  std::cout << "\n=== System Stress High Load Test ===" << std::endl;

  const int num_threads = 8;
  const int operations_per_thread = 1000;
  const size_t data_size = 4096;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        // Simulate high-load operations
        std::string data = generate_test_data(data_size);
        std::sort(data.begin(), data.end());
        std::reverse(data.begin(), data.end());
        completed_operations++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(completed_operations.load()) / (duration.count() / 1000000.0);

  std::cout << "System stress high load:" << std::endl;
  std::cout << "  Threads: " << num_threads << std::endl;
  std::cout << "  Operations per thread: " << operations_per_thread << std::endl;
  std::cout << "  Total operations: " << completed_operations.load() << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
  EXPECT_GT(throughput, 100);
}

/**
 * @brief Test memory stress with large allocations
 */
TEST_F(PerformanceIntegratedTest, MemoryStressLargeAllocations) {
  std::cout << "\n=== Memory Stress Large Allocations Test ===" << std::endl;

  auto& pool = GlobalMemoryPool::instance();
  const int num_allocations = 100;
  const size_t buffer_size = 1024 * 1024;  // 1MB

  auto start_time = std::chrono::high_resolution_clock::now();

  std::vector<std::unique_ptr<uint8_t[]>> buffers;
  buffers.reserve(num_allocations);

  // Allocate large buffers
  for (int i = 0; i < num_allocations; ++i) {
    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      buffers.push_back(std::move(buffer));
    }
  }

  // Release buffers
  for (auto& buffer : buffers) {
    pool.release(std::move(buffer), buffer_size);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_allocations) / (duration.count() / 1000000.0);

  std::cout << "Memory stress large allocations:" << std::endl;
  std::cout << "  Allocations: " << num_allocations << std::endl;
  std::cout << "  Buffer size: " << buffer_size << " bytes" << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " allocations/sec" << std::endl;

  EXPECT_GT(throughput, 10);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}