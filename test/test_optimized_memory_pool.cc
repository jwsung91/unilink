#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "unilink/common/memory_pool.hpp"
#include "unilink/common/optimized_memory_pool.hpp"

using namespace unilink::common;

class OptimizedMemoryPoolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Test setup
  }

  void TearDown() override {
    // Test cleanup
  }
};

TEST_F(OptimizedMemoryPoolTest, BasicFunctionality) {
  OptimizedMemoryPool pool;

  // Test small buffer (1KB)
  auto small_buffer = pool.acquire(1024);
  ASSERT_TRUE(small_buffer != nullptr);
  pool.release(std::move(small_buffer), 1024);

  // Test medium buffer (16KB)
  auto medium_buffer = pool.acquire(16384);
  ASSERT_TRUE(medium_buffer != nullptr);
  pool.release(std::move(medium_buffer), 16384);

  // Test large buffer (128KB)
  auto large_buffer = pool.acquire(131072);
  ASSERT_TRUE(large_buffer != nullptr);
  pool.release(std::move(large_buffer), 131072);
}

TEST_F(OptimizedMemoryPoolTest, SizeCategoryClassification) {
  OptimizedMemoryPool pool;

  // Test size category classification
  EXPECT_EQ(pool.get_size_category(1024), OptimizedMemoryPool::SizeCategory::SMALL);
  EXPECT_EQ(pool.get_size_category(4096), OptimizedMemoryPool::SizeCategory::SMALL);
  EXPECT_EQ(pool.get_size_category(8192), OptimizedMemoryPool::SizeCategory::MEDIUM);
  EXPECT_EQ(pool.get_size_category(32768), OptimizedMemoryPool::SizeCategory::MEDIUM);
  EXPECT_EQ(pool.get_size_category(65536), OptimizedMemoryPool::SizeCategory::LARGE);
  EXPECT_EQ(pool.get_size_category(131072), OptimizedMemoryPool::SizeCategory::LARGE);
}

TEST_F(OptimizedMemoryPoolTest, PerformanceComparison) {
  const size_t num_operations = 10000;
  const size_t buffer_size = 4096;  // 4KB - medium size

  // Test standard memory pool
  MemoryPool standard_pool(400, 2000);
  auto start_time = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_operations; ++i) {
    auto buffer = standard_pool.acquire(buffer_size);
    if (buffer) {
      standard_pool.release(std::move(buffer), buffer_size);
    }
  }

  auto standard_duration = std::chrono::high_resolution_clock::now() - start_time;
  auto standard_stats = standard_pool.get_stats();

  // Test optimized memory pool
  OptimizedMemoryPool optimized_pool;
  start_time = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_operations; ++i) {
    auto buffer = optimized_pool.acquire(buffer_size);
    if (buffer) {
      optimized_pool.release(std::move(buffer), buffer_size);
    }
  }

  auto optimized_duration = std::chrono::high_resolution_clock::now() - start_time;
  auto optimized_stats = optimized_pool.get_stats();

  // Performance comparison
  double standard_throughput =
      (num_operations * 1000000.0) / std::chrono::duration_cast<std::chrono::microseconds>(standard_duration).count();
  double optimized_throughput =
      (num_operations * 1000000.0) / std::chrono::duration_cast<std::chrono::microseconds>(optimized_duration).count();

  std::cout << "Standard Pool - Throughput: " << standard_throughput
            << " ops/sec, Hit Rate: " << (standard_pool.get_hit_rate() * 100.0) << "%" << std::endl;
  std::cout << "Optimized Pool - Throughput: " << optimized_throughput
            << " ops/sec, Hit Rate: " << (optimized_pool.get_hit_rate() * 100.0) << "%" << std::endl;

  // Optimized pool should perform better or equal
  EXPECT_GE(optimized_throughput, standard_throughput * 0.8);  // Allow 20% tolerance
}

TEST_F(OptimizedMemoryPoolTest, ConcurrentAccess) {
  OptimizedMemoryPool pool;
  const int num_threads = 4;
  const int operations_per_thread = 1000;
  const size_t buffer_size = 2048;

  std::atomic<size_t> success_count{0};
  std::atomic<size_t> total_operations{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&pool, &success_count, &total_operations, operations_per_thread, buffer_size]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        total_operations++;

        auto buffer = pool.acquire(buffer_size);
        if (buffer) {
          pool.release(std::move(buffer), buffer_size);
          success_count++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  double success_rate = (100.0 * success_count.load()) / total_operations.load();
  std::cout << "Concurrent Access - Success Rate: " << success_rate << "%" << std::endl;

  EXPECT_GE(success_rate, 95.0);  // At least 95% success rate
}

TEST_F(OptimizedMemoryPoolTest, OptimizedPooledBuffer) {
  OptimizedMemoryPool pool;

  // Test RAII wrapper
  {
    OptimizedPooledBuffer buffer(4096, pool);
    EXPECT_TRUE(buffer.valid());
    EXPECT_EQ(buffer.size(), 4096);
    EXPECT_NE(buffer.data(), nullptr);
  }

  // Buffer should be automatically released
  auto stats = pool.get_stats();
  EXPECT_GT(stats.total_allocations, 0);
}

TEST_F(OptimizedMemoryPoolTest, GlobalOptimizedMemoryPool) {
  auto& global_pool = GlobalOptimizedMemoryPool::instance();

  auto buffer = global_pool.acquire(1024);
  EXPECT_TRUE(buffer != nullptr);
  global_pool.release(std::move(buffer), 1024);

  auto stats = global_pool.get_stats();
  EXPECT_GT(stats.total_allocations, 0);
}

TEST_F(OptimizedMemoryPoolTest, SizeSpecificPerformance) {
  OptimizedMemoryPool pool;
  const size_t num_operations = 5000;

  // Test different buffer sizes
  std::vector<size_t> sizes = {1024, 4096, 16384, 65536};

  for (size_t size : sizes) {
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_operations; ++i) {
      auto buffer = pool.acquire(size);
      if (buffer) {
        pool.release(std::move(buffer), size);
      }
    }

    auto duration = std::chrono::high_resolution_clock::now() - start_time;
    double throughput =
        (num_operations * 1000000.0) / std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

    auto category_stats = pool.get_stats(pool.get_size_category(size));
    double hit_rate = pool.get_hit_rate(pool.get_size_category(size));

    std::cout << "Size " << size << " bytes - Throughput: " << throughput
              << " ops/sec, Hit Rate: " << (hit_rate * 100.0) << "%" << std::endl;

    // Each size should have reasonable performance
    EXPECT_GT(throughput, 1000);  // At least 1K ops/sec
    EXPECT_GT(hit_rate, 0.5);     // At least 50% hit rate
  }
}

TEST_F(OptimizedMemoryPoolTest, MemoryUsage) {
  OptimizedMemoryPool pool;

  // Allocate some buffers
  std::vector<std::unique_ptr<uint8_t[]>> buffers;
  for (int i = 0; i < 100; ++i) {
    auto buffer = pool.acquire(1024);
    if (buffer) {
      buffers.push_back(std::move(buffer));
    }
  }

  auto memory_usage = pool.get_memory_usage();
  EXPECT_GT(memory_usage.first, 0);   // Used memory
  EXPECT_GT(memory_usage.second, 0);  // Total allocated memory

  // Release buffers
  for (auto& buffer : buffers) {
    pool.release(std::move(buffer), 1024);
  }

  std::cout << "Memory Usage - Used: " << memory_usage.first << " bytes, Total: " << memory_usage.second << " bytes"
            << std::endl;
}

TEST_F(OptimizedMemoryPoolTest, HealthMetrics) {
  OptimizedMemoryPool pool;

  // Perform some operations
  for (int i = 0; i < 1000; ++i) {
    auto buffer = pool.acquire(2048);
    if (buffer) {
      pool.release(std::move(buffer), 2048);
    }
  }

  auto health = pool.get_health_metrics();

  EXPECT_GE(health.pool_utilization, 0.0);
  EXPECT_LE(health.pool_utilization, 1.0);
  EXPECT_GE(health.hit_rate, 0.0);
  EXPECT_LE(health.hit_rate, 1.0);
  EXPECT_GE(health.memory_efficiency, 0.0);
  EXPECT_LE(health.memory_efficiency, 1.0);
  EXPECT_GE(health.performance_score, 0.0);
  EXPECT_LE(health.performance_score, 1.0);

  std::cout << "Health Metrics - Utilization: " << (health.pool_utilization * 100.0)
            << "%, Hit Rate: " << (health.hit_rate * 100.0) << "%, Efficiency: " << (health.memory_efficiency * 100.0)
            << "%, Performance: " << (health.performance_score * 100.0) << "%" << std::endl;
}
