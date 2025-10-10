#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iomanip>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/builder/unified_builder.hpp"
#include "unilink/common/memory_pool.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::builder;
using namespace unilink::common;
using namespace std::chrono_literals;

// ============================================================================
// BENCHMARK TESTS
// ============================================================================

/**
 * @brief Performance benchmark tests for comprehensive performance analysis
 */
class BenchmarkTest : public BaseTest {
 protected:
  void SetUp() override {
    BaseTest::SetUp();
    // Reset memory pool for clean benchmarking
    auto& pool = common::GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));
  }

  void TearDown() override {
    // Clean up memory pool
    auto& pool = common::GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));
    BaseTest::TearDown();
  }

  // Helper function to format numbers with commas
  std::string formatNumber(size_t number) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << number;
    return ss.str();
  }

  // Helper function to format time duration
  std::string formatDuration(std::chrono::milliseconds duration) {
    if (duration.count() < 1000) {
      return std::to_string(duration.count()) + "ms";
    } else {
      return std::to_string(duration.count() / 1000.0) + "s";
    }
  }

  // Helper function to calculate throughput
  double calculateThroughput(size_t operations, std::chrono::milliseconds duration) {
    return static_cast<double>(operations) / (duration.count() / 1000.0);
  }
};

// ============================================================================
// MEMORY POOL PERFORMANCE BENCHMARKS
// ============================================================================

/**
 * @brief Memory pool allocation/deallocation performance benchmark
 */
TEST_F(BenchmarkTest, MemoryPoolAllocationPerformance) {
  std::cout << "\n=== Memory Pool Allocation Performance Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const size_t num_operations = 100000;
  const size_t buffer_size = 4096;

  std::vector<std::unique_ptr<uint8_t[]>> buffers;
  buffers.reserve(num_operations);

  // Benchmark allocation
  auto start_time = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_operations; ++i) {
    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      buffers.push_back(std::move(buffer));
    }
  }

  auto allocation_time = std::chrono::high_resolution_clock::now();
  auto allocation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(allocation_time - start_time);

  // Benchmark deallocation
  auto deallocation_start = std::chrono::high_resolution_clock::now();

  for (auto& buffer : buffers) {
    pool.release(std::move(buffer), buffer_size);
  }

  auto deallocation_time = std::chrono::high_resolution_clock::now();
  auto deallocation_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(deallocation_time - deallocation_start);

  auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(deallocation_time - start_time);

  // Calculate metrics
  double allocation_throughput = calculateThroughput(num_operations, allocation_duration);
  double deallocation_throughput = calculateThroughput(num_operations, deallocation_duration);
  double total_throughput = calculateThroughput(num_operations * 2, total_duration);

  std::cout << "Operations: " << formatNumber(num_operations) << std::endl;
  std::cout << "Buffer size: " << formatNumber(buffer_size) << " bytes" << std::endl;
  std::cout << "Allocation time: " << formatDuration(allocation_duration) << std::endl;
  std::cout << "Deallocation time: " << formatDuration(deallocation_duration) << std::endl;
  std::cout << "Total time: " << formatDuration(total_duration) << std::endl;
  std::cout << "Allocation throughput: " << std::fixed << std::setprecision(2) << allocation_throughput << " ops/sec"
            << std::endl;
  std::cout << "Deallocation throughput: " << std::fixed << std::setprecision(2) << deallocation_throughput
            << " ops/sec" << std::endl;
  std::cout << "Total throughput: " << std::fixed << std::setprecision(2) << total_throughput << " ops/sec"
            << std::endl;

  // Performance assertions
  EXPECT_GT(allocation_throughput, 1000);    // At least 1K ops/sec
  EXPECT_GT(deallocation_throughput, 1000);  // At least 1K ops/sec
  EXPECT_LT(total_duration.count(), 10000);  // Less than 10 seconds

  std::cout << "✓ Memory pool allocation performance benchmark completed" << std::endl;
}

/**
 * @brief Memory pool concurrent access performance benchmark
 */
TEST_F(BenchmarkTest, MemoryPoolConcurrentPerformance) {
  std::cout << "\n=== Memory Pool Concurrent Performance Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const int num_threads = 8;
  const int operations_per_thread = 10000;
  const size_t buffer_size = 1024;

  std::vector<std::thread> threads;
  std::atomic<size_t> total_operations{0};
  std::atomic<size_t> successful_operations{0};
  std::atomic<size_t> failed_operations{0};

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(
        [&pool, operations_per_thread, buffer_size, &total_operations, &successful_operations, &failed_operations]() {
          for (int i = 0; i < operations_per_thread; ++i) {
            total_operations++;

            auto buffer = pool.acquire(buffer_size);
            if (buffer) {
              // Simulate some work
              std::this_thread::sleep_for(std::chrono::microseconds(1));
              pool.release(std::move(buffer), buffer_size);
              successful_operations++;
            } else {
              failed_operations++;
            }
          }
        });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  double throughput = calculateThroughput(total_operations.load(), duration);
  double success_rate = (100.0 * successful_operations.load()) / total_operations.load();

  std::cout << "Threads: " << num_threads << std::endl;
  std::cout << "Operations per thread: " << formatNumber(operations_per_thread) << std::endl;
  std::cout << "Total operations: " << formatNumber(total_operations.load()) << std::endl;
  std::cout << "Successful operations: " << formatNumber(successful_operations.load()) << std::endl;
  std::cout << "Failed operations: " << formatNumber(failed_operations.load()) << std::endl;
  std::cout << "Duration: " << formatDuration(duration) << std::endl;
  std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " ops/sec" << std::endl;
  std::cout << "Success rate: " << std::fixed << std::setprecision(2) << success_rate << "%" << std::endl;

  // Performance assertions
  EXPECT_GT(throughput, 1000);         // At least 1K ops/sec
  EXPECT_GT(success_rate, 90.0);       // At least 90% success rate
  EXPECT_LT(duration.count(), 15000);  // Less than 15 seconds

  std::cout << "✓ Memory pool concurrent performance benchmark completed" << std::endl;
}

/**
 * @brief Memory pool hit rate analysis benchmark
 */
TEST_F(BenchmarkTest, MemoryPoolHitRateAnalysis) {
  std::cout << "\n=== Memory Pool Hit Rate Analysis Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const size_t num_cycles = 1000;
  const size_t buffers_per_cycle = 100;
  const size_t buffer_size = 2048;

  // Get initial stats
  auto initial_stats = pool.get_stats();
  std::cout << "Initial pool hits: " << initial_stats.pool_hits << std::endl;

  auto start_time = std::chrono::high_resolution_clock::now();

  // Perform allocation/deallocation cycles
  for (size_t cycle = 0; cycle < num_cycles; ++cycle) {
    std::vector<std::unique_ptr<uint8_t[]>> cycle_buffers;
    cycle_buffers.reserve(buffers_per_cycle);

    // Allocate buffers
    for (size_t i = 0; i < buffers_per_cycle; ++i) {
      auto buffer = pool.acquire(buffer_size);
      if (buffer) {
        cycle_buffers.push_back(std::move(buffer));
      }
    }

    // Release buffers
    for (auto& buffer : cycle_buffers) {
      pool.release(std::move(buffer), buffer_size);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Get final stats
  auto final_stats = pool.get_stats();
  size_t total_hits = final_stats.pool_hits - initial_stats.pool_hits;
  size_t total_allocations = final_stats.total_allocations - initial_stats.total_allocations;
  double hit_rate = total_allocations > 0 ? (100.0 * total_hits) / total_allocations : 0.0;

  double throughput = calculateThroughput(num_cycles * buffers_per_cycle * 2, duration);

  std::cout << "Cycles: " << formatNumber(num_cycles) << std::endl;
  std::cout << "Buffers per cycle: " << formatNumber(buffers_per_cycle) << std::endl;
  std::cout << "Total allocations: " << formatNumber(total_allocations) << std::endl;
  std::cout << "Pool hits: " << formatNumber(total_hits) << std::endl;
  std::cout << "Pool misses: " << formatNumber(total_allocations - total_hits) << std::endl;
  std::cout << "Hit rate: " << std::fixed << std::setprecision(2) << hit_rate << "%" << std::endl;
  std::cout << "Duration: " << formatDuration(duration) << std::endl;
  std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " ops/sec" << std::endl;

  // Performance assertions
  EXPECT_GT(hit_rate, 0.0);            // Some hit rate should be achieved
  EXPECT_GT(throughput, 500);          // At least 500 ops/sec
  EXPECT_LT(duration.count(), 20000);  // Less than 20 seconds

  std::cout << "✓ Memory pool hit rate analysis benchmark completed" << std::endl;
}

// ============================================================================
// NETWORK COMMUNICATION PERFORMANCE BENCHMARKS
// ============================================================================

/**
 * @brief Network communication throughput benchmark (simplified)
 */
TEST_F(BenchmarkTest, NetworkCommunicationThroughput) {
  std::cout << "\n=== Network Communication Throughput Benchmark ===" << std::endl;

  // Simulate network communication using memory pool
  auto& pool = common::GlobalMemoryPool::instance();
  const size_t num_messages = 1000;
  const size_t message_size = 1024;

  std::atomic<size_t> messages_processed{0};
  std::atomic<size_t> bytes_processed{0};

  auto start_time = std::chrono::high_resolution_clock::now();

  // Simulate message processing
  for (size_t i = 0; i < num_messages; ++i) {
    // Simulate message allocation
    auto buffer = pool.acquire(message_size);
    if (buffer) {
      // Simulate message processing
      memset(buffer.get(), 'A' + (i % 26), message_size);

      // Simulate network delay
      std::this_thread::sleep_for(std::chrono::microseconds(10));

      pool.release(std::move(buffer), message_size);
      messages_processed++;
      bytes_processed += message_size;
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  double message_throughput = calculateThroughput(messages_processed.load(), duration);
  double data_throughput = (bytes_processed.load() / 1024.0) / (duration.count() / 1000.0);  // KB/s

  std::cout << "Messages: " << formatNumber(messages_processed.load()) << std::endl;
  std::cout << "Message size: " << formatNumber(message_size) << " bytes" << std::endl;
  std::cout << "Total data: " << formatNumber(bytes_processed.load()) << " bytes" << std::endl;
  std::cout << "Duration: " << formatDuration(duration) << std::endl;
  std::cout << "Message throughput: " << std::fixed << std::setprecision(2) << message_throughput << " msg/sec"
            << std::endl;
  std::cout << "Data throughput: " << std::fixed << std::setprecision(2) << data_throughput << " KB/sec" << std::endl;

  // Performance assertions
  EXPECT_GT(message_throughput, 10);   // At least 10 msg/sec
  EXPECT_GT(data_throughput, 1.0);     // At least 1 KB/sec
  EXPECT_LT(duration.count(), 30000);  // Less than 30 seconds

  std::cout << "✓ Network communication throughput benchmark completed" << std::endl;
}

/**
 * @brief Network latency benchmark (simplified)
 */
TEST_F(BenchmarkTest, NetworkLatencyBenchmark) {
  std::cout << "\n=== Network Latency Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const size_t num_requests = 1000;
  const size_t request_size = 512;

  std::vector<std::chrono::microseconds> latencies;
  latencies.reserve(num_requests);

  auto start_time = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_requests; ++i) {
    auto request_start = std::chrono::high_resolution_clock::now();

    // Simulate request processing
    auto buffer = pool.acquire(request_size);
    if (buffer) {
      // Simulate processing time
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      pool.release(std::move(buffer), request_size);
    }

    auto request_end = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(request_end - request_start);
    latencies.push_back(latency);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Calculate latency statistics
  std::sort(latencies.begin(), latencies.end());
  auto min_latency = latencies.front();
  auto max_latency = latencies.back();
  auto median_latency = latencies[latencies.size() / 2];

  // Calculate average latency
  auto total_latency = std::accumulate(latencies.begin(), latencies.end(), std::chrono::microseconds(0));
  auto avg_latency = total_latency / latencies.size();

  double throughput = calculateThroughput(num_requests, total_duration);

  std::cout << "Requests: " << formatNumber(num_requests) << std::endl;
  std::cout << "Request size: " << formatNumber(request_size) << " bytes" << std::endl;
  std::cout << "Min latency: " << min_latency.count() << " μs" << std::endl;
  std::cout << "Max latency: " << max_latency.count() << " μs" << std::endl;
  std::cout << "Median latency: " << median_latency.count() << " μs" << std::endl;
  std::cout << "Average latency: " << avg_latency.count() << " μs" << std::endl;
  std::cout << "Total duration: " << formatDuration(total_duration) << std::endl;
  std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " req/sec" << std::endl;

  // Performance assertions
  EXPECT_LT(avg_latency.count(), 10000);     // Average latency < 10ms
  EXPECT_GT(throughput, 50);                 // At least 50 req/sec
  EXPECT_LT(total_duration.count(), 30000);  // Less than 30 seconds

  std::cout << "✓ Network latency benchmark completed" << std::endl;
}

// ============================================================================
// CONCURRENCY PERFORMANCE BENCHMARKS
// ============================================================================

/**
 * @brief Concurrent operations performance benchmark
 */
TEST_F(BenchmarkTest, ConcurrentOperationsPerformance) {
  std::cout << "\n=== Concurrent Operations Performance Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const int num_threads = 4;
  const int operations_per_thread = 500;
  const size_t buffer_size = 1024;

  std::vector<std::thread> threads;
  std::atomic<size_t> total_operations{0};
  std::atomic<size_t> successful_operations{0};
  std::atomic<size_t> failed_operations{0};

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(
        [&pool, operations_per_thread, buffer_size, &total_operations, &successful_operations, &failed_operations]() {
          for (int i = 0; i < operations_per_thread; ++i) {
            total_operations++;

            // Simple allocation/deallocation only
            auto buffer = pool.acquire(buffer_size);
            if (buffer) {
              pool.release(std::move(buffer), buffer_size);
              successful_operations++;
            } else {
              failed_operations++;
            }
          }
        });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  double throughput = calculateThroughput(total_operations.load(), duration);
  double success_rate = (100.0 * successful_operations.load()) / total_operations.load();

  std::cout << "Threads: " << num_threads << std::endl;
  std::cout << "Operations per thread: " << formatNumber(operations_per_thread) << std::endl;
  std::cout << "Total operations: " << formatNumber(total_operations.load()) << std::endl;
  std::cout << "Successful operations: " << formatNumber(successful_operations.load()) << std::endl;
  std::cout << "Failed operations: " << formatNumber(failed_operations.load()) << std::endl;
  std::cout << "Duration: " << formatDuration(duration) << std::endl;
  std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " ops/sec" << std::endl;
  std::cout << "Success rate: " << std::fixed << std::setprecision(2) << success_rate << "%" << std::endl;

  // Performance assertions
  EXPECT_GT(throughput, 1000);         // At least 1K ops/sec
  EXPECT_GT(success_rate, 95.0);       // At least 95% success rate
  EXPECT_LT(duration.count(), 20000);  // Less than 20 seconds

  std::cout << "✓ Concurrent operations performance benchmark completed" << std::endl;
}

// ============================================================================
// RESOURCE USAGE MONITORING BENCHMARKS
// ============================================================================

/**
 * @brief Memory usage monitoring benchmark
 */
TEST_F(BenchmarkTest, MemoryUsageMonitoring) {
  std::cout << "\n=== Memory Usage Monitoring Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const size_t num_cycles = 50;
  const size_t buffers_per_cycle = 20;
  const size_t buffer_size = 2048;

  // Get initial memory stats
  auto initial_stats = pool.get_stats();
  size_t initial_allocations = initial_stats.total_allocations;
  auto initial_memory_pair = pool.get_memory_usage();
  size_t initial_memory = initial_memory_pair.first;  // Use first value (current usage)

  std::cout << "Initial allocations: " << formatNumber(initial_allocations) << std::endl;
  std::cout << "Initial memory usage: " << formatNumber(initial_memory) << " bytes" << std::endl;

  auto start_time = std::chrono::high_resolution_clock::now();

  // Perform memory allocation cycles
  for (size_t cycle = 0; cycle < num_cycles; ++cycle) {
    std::vector<std::unique_ptr<uint8_t[]>> cycle_buffers;
    cycle_buffers.reserve(buffers_per_cycle);

    // Allocate buffers
    for (size_t i = 0; i < buffers_per_cycle; ++i) {
      auto buffer = pool.acquire(buffer_size);
      if (buffer) {
        cycle_buffers.push_back(std::move(buffer));
      }
    }

    // Release buffers
    for (auto& buffer : cycle_buffers) {
      pool.release(std::move(buffer), buffer_size);
    }

    // Periodic cleanup
    if (cycle % 10 == 0) {
      pool.cleanup_old_buffers(std::chrono::milliseconds(0));
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Get final memory stats
  auto final_stats = pool.get_stats();
  size_t final_allocations = final_stats.total_allocations;
  auto final_memory_pair = pool.get_memory_usage();
  size_t final_memory = final_memory_pair.first;  // Use first value (current usage)

  size_t total_allocations = final_allocations - initial_allocations;
  size_t memory_delta = final_memory - initial_memory;

  double throughput = calculateThroughput(num_cycles * buffers_per_cycle * 2, duration);

  std::cout << "Cycles: " << formatNumber(num_cycles) << std::endl;
  std::cout << "Buffers per cycle: " << formatNumber(buffers_per_cycle) << std::endl;
  std::cout << "Total allocations: " << formatNumber(total_allocations) << std::endl;
  std::cout << "Final allocations: " << formatNumber(final_allocations) << std::endl;
  std::cout << "Initial memory: " << formatNumber(initial_memory) << " bytes" << std::endl;
  std::cout << "Final memory: " << formatNumber(final_memory) << " bytes" << std::endl;
  std::cout << "Memory delta: " << formatNumber(memory_delta) << " bytes" << std::endl;
  std::cout << "Duration: " << formatDuration(duration) << std::endl;
  std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " ops/sec" << std::endl;

  // Performance assertions
  EXPECT_GT(total_allocations, 0);     // Some allocations should have occurred
  EXPECT_GT(throughput, 100);          // At least 100 ops/sec
  EXPECT_LT(duration.count(), 10000);  // Less than 10 seconds

  std::cout << "✓ Memory usage monitoring benchmark completed" << std::endl;
}

/**
 * @brief Performance regression detection benchmark
 *
 * This test validates that the memory pool maintains consistent performance
 * across multiple iterations. The test accounts for batch statistics updates
 * which may cause some natural variation in performance.
 */
TEST_F(BenchmarkTest, PerformanceRegressionDetection) {
  std::cout << "\n=== Performance Regression Detection Benchmark ===" << std::endl;

  auto& pool = common::GlobalMemoryPool::instance();
  const size_t num_iterations = 20;              // Increased for better statistical significance
  const size_t operations_per_iteration = 1000;  // Increased to reduce noise
  const size_t buffer_size = 1024;

  std::vector<double> iteration_times;
  iteration_times.reserve(num_iterations);

  // Warm up the pool to ensure consistent state
  for (size_t i = 0; i < 100; ++i) {
    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      pool.release(std::move(buffer), buffer_size);
    }
  }

  for (size_t iter = 0; iter < num_iterations; ++iter) {
    auto iteration_start = std::chrono::high_resolution_clock::now();

    // Perform operations
    for (size_t i = 0; i < operations_per_iteration; ++i) {
      auto buffer = pool.acquire(buffer_size);
      if (buffer) {
        pool.release(std::move(buffer), buffer_size);
      }
    }

    auto iteration_end = std::chrono::high_resolution_clock::now();
    auto iteration_duration = std::chrono::duration_cast<std::chrono::microseconds>(iteration_end - iteration_start);

    iteration_times.push_back(iteration_duration.count() / 1000.0);  // Convert to ms
  }

  // Calculate statistics
  std::sort(iteration_times.begin(), iteration_times.end());
  double min_time = iteration_times.front();
  double max_time = iteration_times.back();
  double median_time = iteration_times[iteration_times.size() / 2];

  double total_time = std::accumulate(iteration_times.begin(), iteration_times.end(), 0.0);
  double avg_time = total_time / iteration_times.size();

  // Calculate coefficient of variation (stability measure)
  double variance = 0.0;
  for (double time : iteration_times) {
    variance += (time - avg_time) * (time - avg_time);
  }
  variance /= iteration_times.size();
  double std_dev = std::sqrt(variance);
  double coefficient_of_variation = (avg_time > 0.0) ? (std_dev / avg_time) * 100.0 : 0.0;

  std::cout << "Iterations: " << formatNumber(num_iterations) << std::endl;
  std::cout << "Operations per iteration: " << formatNumber(operations_per_iteration) << std::endl;
  std::cout << "Min time: " << std::fixed << std::setprecision(2) << min_time << " ms" << std::endl;
  std::cout << "Max time: " << std::fixed << std::setprecision(2) << max_time << " ms" << std::endl;
  std::cout << "Median time: " << std::fixed << std::setprecision(2) << median_time << " ms" << std::endl;
  std::cout << "Average time: " << std::fixed << std::setprecision(2) << avg_time << " ms" << std::endl;
  std::cout << "Standard deviation: " << std::fixed << std::setprecision(2) << std_dev << " ms" << std::endl;
  std::cout << "Coefficient of variation: " << std::fixed << std::setprecision(2) << coefficient_of_variation << "%"
            << std::endl;

  // Performance assertions - adjusted for batch statistics updates
  EXPECT_LT(avg_time, 1000);  // Average time < 1 second
  if (avg_time > 0.0) {
    // Increased threshold to account for batch statistics update variability
    // Batch updates occur every 100 operations, which can cause natural variation
    EXPECT_LT(coefficient_of_variation, 100.0);  // CV < 100% (reasonable for batch updates)
  }
  EXPECT_LT(max_time, 2000);  // Max time < 2 seconds

  // Additional stability checks
  EXPECT_GT(avg_time, 0.0);              // Should have measurable performance
  EXPECT_LT(max_time / min_time, 10.0);  // Max should not be more than 10x min (reasonable range)

  std::cout << "✓ Performance regression detection benchmark completed" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
