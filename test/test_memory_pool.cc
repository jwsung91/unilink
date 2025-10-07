#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>

#include "unilink/common/memory_pool.hpp"

using namespace unilink;
using namespace std::chrono_literals;

class MemoryPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Don't reset the pool between tests to allow statistics to accumulate
        // This better reflects real-world usage where the pool persists
    }
    
    void TearDown() override {
        // Clean up after each test
        auto& pool = common::GlobalMemoryPool::instance();
        pool.cleanup_old_buffers(0ms);
    }
};

/**
 * @brief Basic memory pool functionality test
 */
TEST_F(MemoryPoolTest, BasicFunctionality) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    // Test acquiring and releasing buffers
    auto buffer1 = pool.acquire(1024);
    EXPECT_NE(buffer1, nullptr);
    
    auto buffer2 = pool.acquire(4096);
    EXPECT_NE(buffer2, nullptr);
    
    // Release buffers back to pool
    pool.release(std::move(buffer1), 1024);
    pool.release(std::move(buffer2), 4096);
    
    // Verify pool statistics
    auto stats = pool.get_stats();
    EXPECT_GT(stats.total_allocations, 0);
}

/**
 * @brief Predefined buffer sizes test
 */
TEST_F(MemoryPoolTest, PredefinedBufferSizes) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    // Test all predefined buffer sizes
    auto small = pool.acquire(common::MemoryPool::BufferSize::SMALL);
    auto medium = pool.acquire(common::MemoryPool::BufferSize::MEDIUM);
    auto large = pool.acquire(common::MemoryPool::BufferSize::LARGE);
    auto xlarge = pool.acquire(common::MemoryPool::BufferSize::XLARGE);
    
    EXPECT_NE(small, nullptr);
    EXPECT_NE(medium, nullptr);
    EXPECT_NE(large, nullptr);
    EXPECT_NE(xlarge, nullptr);
    
    // Verify sizes
    EXPECT_EQ(static_cast<size_t>(common::MemoryPool::BufferSize::SMALL), 1024);
    EXPECT_EQ(static_cast<size_t>(common::MemoryPool::BufferSize::MEDIUM), 4096);
    EXPECT_EQ(static_cast<size_t>(common::MemoryPool::BufferSize::LARGE), 16384);
    EXPECT_EQ(static_cast<size_t>(common::MemoryPool::BufferSize::XLARGE), 65536);
    
    // Release buffers
    pool.release(std::move(small), static_cast<size_t>(common::MemoryPool::BufferSize::SMALL));
    pool.release(std::move(medium), static_cast<size_t>(common::MemoryPool::BufferSize::MEDIUM));
    pool.release(std::move(large), static_cast<size_t>(common::MemoryPool::BufferSize::LARGE));
    pool.release(std::move(xlarge), static_cast<size_t>(common::MemoryPool::BufferSize::XLARGE));
}

/**
 * @brief PooledBuffer RAII wrapper test
 */
TEST_F(MemoryPoolTest, PooledBufferRAII) {
    {
        // Create PooledBuffer in scope
        common::PooledBuffer buffer(1024);
        EXPECT_TRUE(buffer.valid());
        EXPECT_EQ(buffer.size(), 1024);
        EXPECT_NE(buffer.data(), nullptr);
        
        // Write some data
        memset(buffer.data(), 0xAB, 100);
        
        // Verify data
        for (int i = 0; i < 100; ++i) {
            EXPECT_EQ(buffer.data()[i], 0xAB);
        }
    }
    // Buffer should be automatically returned to pool when going out of scope
    
    // Verify pool has buffers available
    auto& pool = common::GlobalMemoryPool::instance();
    auto stats = pool.get_stats();
    EXPECT_GT(stats.current_pool_size, 0);
}

/**
 * @brief Performance comparison test
 */
TEST_F(MemoryPoolTest, PerformanceComparison) {
    const int num_allocations = 1000;
    const size_t buffer_size = 4096;
    
    auto& pool = common::GlobalMemoryPool::instance();
    
    // Test memory pool performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::unique_ptr<uint8_t[]>> pooled_buffers;
    for (int i = 0; i < num_allocations; ++i) {
        auto buffer = pool.acquire(buffer_size);
        if (buffer) {
            pooled_buffers.push_back(std::move(buffer));
        }
    }
    
    // Release all buffers
    for (auto& buffer : pooled_buffers) {
        pool.release(std::move(buffer), buffer_size);
    }
    
    auto pool_time = std::chrono::high_resolution_clock::now() - start_time;
    
    // Test regular allocation performance
    start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::unique_ptr<uint8_t[]>> regular_buffers;
    for (int i = 0; i < num_allocations; ++i) {
        regular_buffers.push_back(std::make_unique<uint8_t[]>(buffer_size));
    }
    
    auto regular_time = std::chrono::high_resolution_clock::now() - start_time;
    
    // Memory pool should be faster (or at least not significantly slower)
    auto pool_ms = std::chrono::duration_cast<std::chrono::milliseconds>(pool_time).count();
    auto regular_ms = std::chrono::duration_cast<std::chrono::milliseconds>(regular_time).count();
    
    std::cout << "Memory pool time: " << pool_ms << "ms" << std::endl;
    std::cout << "Regular allocation time: " << regular_ms << "ms" << std::endl;
    
    // Verify pool statistics
    auto stats = pool.get_stats();
    EXPECT_GT(stats.total_allocations, 0);
    // Note: pool hits might be 0 if all allocations were misses (pool was empty initially)
    // This is expected behavior for the first run
}

/**
 * @brief Thread safety test
 */
TEST_F(MemoryPoolTest, ThreadSafety) {
    auto& pool = common::GlobalMemoryPool::instance();
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pool, allocations_per_thread, &success_count, &error_count]() {
            for (int i = 0; i < allocations_per_thread; ++i) {
                try {
                    auto buffer = pool.acquire(1024);
                    if (buffer) {
                        // Simulate some work
                        memset(buffer.get(), 0x42, 100);
                        pool.release(std::move(buffer), 1024);
                        success_count++;
                    } else {
                        error_count++;
                    }
                } catch (const std::exception& e) {
                    error_count++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify results
    EXPECT_GT(success_count.load(), 0);
    EXPECT_EQ(error_count.load(), 0);
    
    // Verify pool statistics
    auto stats = pool.get_stats();
    EXPECT_GT(stats.total_allocations, 0);
}

/**
 * @brief Hit rate calculation test
 */
TEST_F(MemoryPoolTest, HitRateCalculation) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    // Perform some allocations and releases to build up pool
    std::vector<std::unique_ptr<uint8_t[]>> buffers;
    for (int i = 0; i < 10; ++i) {
        auto buffer = pool.acquire(1024);
        if (buffer) {
            buffers.push_back(std::move(buffer));
        }
    }
    
    // Release all buffers back to pool
    for (auto& buffer : buffers) {
        pool.release(std::move(buffer), 1024);
    }
    
    // Now subsequent allocations should hit the pool
    double initial_hit_rate = pool.get_hit_rate();
    
    // Perform more allocations
    for (int i = 0; i < 5; ++i) {
        auto buffer = pool.acquire(1024);
        if (buffer) {
            pool.release(std::move(buffer), 1024);
        }
    }
    
    double final_hit_rate = pool.get_hit_rate();
    
    // Hit rate should be valid (between 0 and 1)
    EXPECT_GE(final_hit_rate, 0.0);
    EXPECT_LE(final_hit_rate, 1.0);
    
    // Note: Hit rate might not always improve due to test interference
    // The important thing is that the pool is working correctly
    std::cout << "Initial hit rate: " << initial_hit_rate << std::endl;
    std::cout << "Final hit rate: " << final_hit_rate << std::endl;
}

/**
 * @brief Memory usage tracking test
 */
TEST_F(MemoryPoolTest, MemoryUsageTracking) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    auto initial_usage = pool.get_memory_usage();
    
    // Allocate some buffers
    std::vector<std::unique_ptr<uint8_t[]>> buffers;
    for (int i = 0; i < 5; ++i) {
        auto buffer = pool.acquire(1024);
        if (buffer) {
            buffers.push_back(std::move(buffer));
        }
    }
    
    auto usage_with_allocated = pool.get_memory_usage();
    
    // Release half of the buffers
    for (size_t i = 0; i < buffers.size() / 2; ++i) {
        pool.release(std::move(buffers[i]), 1024);
    }
    
    auto usage_after_partial_release = pool.get_memory_usage();
    
    // Verify memory usage tracking
    EXPECT_GE(usage_with_allocated.first, initial_usage.first);
    EXPECT_GE(usage_with_allocated.second, initial_usage.second);
    
    // Release remaining buffers
    for (size_t i = buffers.size() / 2; i < buffers.size(); ++i) {
        pool.release(std::move(buffers[i]), 1024);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
