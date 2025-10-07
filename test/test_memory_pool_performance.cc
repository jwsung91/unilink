#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <algorithm>
#include <iomanip>

#include "unilink/common/memory_pool.hpp"

using namespace unilink;
using namespace std::chrono_literals;

class MemoryPoolPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset global memory pool for each test
        auto& pool = common::GlobalMemoryPool::instance();
        pool.resize_pool(100);
        pool.cleanup_old_buffers(0ms);
    }
    
    void TearDown() override {
        auto& pool = common::GlobalMemoryPool::instance();
        pool.cleanup_old_buffers(0ms);
    }
};

/**
 * @brief 메모리 풀 성능 벤치마크
 */
TEST_F(MemoryPoolPerformanceTest, PerformanceBenchmark) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const int num_operations = 10000;
    const std::vector<size_t> buffer_sizes = {1024, 4096, 16384, 32768, 65536};
    
    std::cout << "\n=== Memory Pool Performance Benchmark ===" << std::endl;
    
    for (size_t buffer_size : buffer_sizes) {
        std::vector<std::unique_ptr<uint8_t[]>> buffers;
        buffers.reserve(num_operations);
        
        // Test memory pool performance
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_operations; ++i) {
            auto buffer = pool.acquire(buffer_size);
            if (buffer) {
                buffers.push_back(std::move(buffer));
            }
        }
        
        // Release all buffers
        for (auto& buffer : buffers) {
            pool.release(std::move(buffer), buffer_size);
        }
        
        auto pool_time = std::chrono::high_resolution_clock::now() - start_time;
        
        // Test regular allocation performance
        start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::unique_ptr<uint8_t[]>> regular_buffers;
        regular_buffers.reserve(num_operations);
        
        for (int i = 0; i < num_operations; ++i) {
            regular_buffers.push_back(std::make_unique<uint8_t[]>(buffer_size));
        }
        
        auto regular_time = std::chrono::high_resolution_clock::now() - start_time;
        
        auto pool_ms = std::chrono::duration_cast<std::chrono::microseconds>(pool_time).count();
        auto regular_ms = std::chrono::duration_cast<std::chrono::microseconds>(regular_time).count();
        
        double speedup = static_cast<double>(regular_ms) / static_cast<double>(pool_ms);
        
        std::cout << "Buffer size: " << buffer_size << " bytes" << std::endl;
        std::cout << "  Memory pool: " << pool_ms << " μs" << std::endl;
        std::cout << "  Regular alloc: " << regular_ms << " μs" << std::endl;
        std::cout << "  Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
        
        // Verify pool statistics
        auto stats = pool.get_stats();
        std::cout << "  Hit rate: " << std::fixed << std::setprecision(2) 
                  << (pool.get_hit_rate() * 100.0) << "%" << std::endl;
        std::cout << "  Pool size: " << stats.current_pool_size << std::endl;
        std::cout << std::endl;
    }
}

/**
 * @brief 동시성 성능 테스트
 */
TEST_F(MemoryPoolPerformanceTest, ConcurrentPerformanceTest) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const int num_threads = 4;
    const int operations_per_thread = 2500;
    const size_t buffer_size = 4096;
    
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&pool, operations_per_thread, buffer_size, &success_count, &error_count]() {
            std::vector<std::unique_ptr<uint8_t[]>> buffers;
            buffers.reserve(operations_per_thread);
            
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    auto buffer = pool.acquire(buffer_size);
                    if (buffer) {
                        buffers.push_back(std::move(buffer));
                        success_count++;
                    } else {
                        error_count++;
                    }
                } catch (const std::exception& e) {
                    error_count++;
                }
            }
            
            // Release all buffers
            for (auto& buffer : buffers) {
                pool.release(std::move(buffer), buffer_size);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto total_time = std::chrono::high_resolution_clock::now() - start_time;
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count();
    
    std::cout << "\n=== Concurrent Performance Test ===" << std::endl;
    std::cout << "Threads: " << num_threads << std::endl;
    std::cout << "Operations per thread: " << operations_per_thread << std::endl;
    std::cout << "Total operations: " << (num_threads * operations_per_thread) << std::endl;
    std::cout << "Total time: " << total_ms << " ms" << std::endl;
    std::cout << "Operations per second: " << (num_threads * operations_per_thread * 1000) / total_ms << std::endl;
    std::cout << "Success count: " << success_count.load() << std::endl;
    std::cout << "Error count: " << error_count.load() << std::endl;
    
    // Verify results
    EXPECT_GT(success_count.load(), 0);
    EXPECT_EQ(error_count.load(), 0);
}

/**
 * @brief 메모리 사용량 분석
 */
TEST_F(MemoryPoolPerformanceTest, MemoryUsageAnalysis) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const int num_allocations = 1000;
    const size_t buffer_size = 4096;
    
    auto initial_usage = pool.get_memory_usage();
    std::cout << "\n=== Memory Usage Analysis ===" << std::endl;
    std::cout << "Initial memory usage: " << initial_usage.first << " / " << initial_usage.second << " bytes" << std::endl;
    
    std::vector<std::unique_ptr<uint8_t[]>> buffers;
    buffers.reserve(num_allocations);
    
    // Allocate buffers
    for (int i = 0; i < num_allocations; ++i) {
        auto buffer = pool.acquire(buffer_size);
        if (buffer) {
            buffers.push_back(std::move(buffer));
        }
    }
    
    auto usage_after_alloc = pool.get_memory_usage();
    std::cout << "After allocation: " << usage_after_alloc.first << " / " << usage_after_alloc.second << " bytes" << std::endl;
    
    // Release half
    for (size_t i = 0; i < buffers.size() / 2; ++i) {
        pool.release(std::move(buffers[i]), buffer_size);
    }
    
    auto usage_after_partial_release = pool.get_memory_usage();
    std::cout << "After partial release: " << usage_after_partial_release.first << " / " << usage_after_partial_release.second << " bytes" << std::endl;
    
    // Release remaining
    for (size_t i = buffers.size() / 2; i < buffers.size(); ++i) {
        pool.release(std::move(buffers[i]), buffer_size);
    }
    
    auto final_usage = pool.get_memory_usage();
    std::cout << "Final usage: " << final_usage.first << " / " << final_usage.second << " bytes" << std::endl;
    
    // Verify memory usage tracking
    EXPECT_GE(usage_after_alloc.first, initial_usage.first);
    EXPECT_GE(usage_after_alloc.second, initial_usage.second);
}

/**
 * @brief 히트율 분석
 */
TEST_F(MemoryPoolPerformanceTest, HitRateAnalysis) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const size_t buffer_size = 4096;
    const int num_cycles = 5;
    const int allocations_per_cycle = 100;
    
    std::cout << "\n=== Hit Rate Analysis ===" << std::endl;
    
    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        std::vector<std::unique_ptr<uint8_t[]>> buffers;
        buffers.reserve(allocations_per_cycle);
        
        // Allocate
        for (int i = 0; i < allocations_per_cycle; ++i) {
            auto buffer = pool.acquire(buffer_size);
            if (buffer) {
                buffers.push_back(std::move(buffer));
            }
        }
        
        // Release
        for (auto& buffer : buffers) {
            pool.release(std::move(buffer), buffer_size);
        }
        
        auto stats = pool.get_stats();
        double hit_rate = pool.get_hit_rate();
        double size_hit_rate = pool.get_hit_rate_for_size(buffer_size);
        
        std::cout << "Cycle " << (cycle + 1) << ": Hit rate = " << std::fixed << std::setprecision(2) 
                  << (hit_rate * 100.0) << "%, Size hit rate = " << (size_hit_rate * 100.0) 
                  << "%, Pool size = " << stats.current_pool_size << std::endl;
    }
}

/**
 * @brief 자동 튜닝 테스트
 */
TEST_F(MemoryPoolPerformanceTest, AutoTuningTest) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const size_t buffer_size = 4096;
    const int num_operations = 1000;
    
    std::cout << "\n=== Auto Tuning Test ===" << std::endl;
    
    // Initial stats
    auto initial_stats = pool.get_detailed_stats();
    std::cout << "Initial hit rate: " << std::fixed << std::setprecision(2) 
              << (pool.get_hit_rate() * 100.0) << "%" << std::endl;
    
    // Perform many operations to trigger auto-tuning
    for (int i = 0; i < num_operations; ++i) {
        auto buffer = pool.acquire(buffer_size);
        if (buffer) {
            pool.release(std::move(buffer), buffer_size);
        }
    }
    
    // Trigger auto-tuning
    pool.auto_tune();
    
    // Check if optimization was applied
    pool.optimize_for_size(buffer_size, 0.8);
    
    auto final_stats = pool.get_detailed_stats();
    double final_hit_rate = pool.get_hit_rate();
    
    std::cout << "Final hit rate: " << std::fixed << std::setprecision(2) 
              << (final_hit_rate * 100.0) << "%" << std::endl;
    std::cout << "Memory usage: " << final_stats.current_memory_usage 
              << " / " << final_stats.peak_memory_usage << " bytes" << std::endl;
    std::cout << "Average allocation time: " << std::fixed << std::setprecision(3) 
              << final_stats.average_allocation_time_ms << " ms" << std::endl;
    
    // Verify improvement
    EXPECT_GT(final_hit_rate, 0.0);
    EXPECT_GT(final_stats.current_memory_usage, 0);
}

/**
 * @brief 상세 통계 테스트
 */
TEST_F(MemoryPoolPerformanceTest, DetailedStatsTest) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const std::vector<size_t> buffer_sizes = {1024, 4096, 16384};
    const int operations_per_size = 100;
    
    std::cout << "\n=== Detailed Stats Test ===" << std::endl;
    
    // Perform operations with different buffer sizes
    for (size_t size : buffer_sizes) {
        for (int i = 0; i < operations_per_size; ++i) {
            auto buffer = pool.acquire(size);
            if (buffer) {
                pool.release(std::move(buffer), size);
            }
        }
    }
    
    auto detailed_stats = pool.get_detailed_stats();
    
    std::cout << "Total allocations: " << detailed_stats.total_allocations << std::endl;
    std::cout << "Pool hits: " << detailed_stats.pool_hits << std::endl;
    std::cout << "Pool misses: " << detailed_stats.pool_misses << std::endl;
    std::cout << "Overall hit rate: " << std::fixed << std::setprecision(2) 
              << (pool.get_hit_rate() * 100.0) << "%" << std::endl;
    
    // Check hit rates by size
    for (size_t size : buffer_sizes) {
        double hit_rate = pool.get_hit_rate_for_size(size);
        std::cout << "Hit rate for " << size << " bytes: " << std::fixed << std::setprecision(2) 
                  << (hit_rate * 100.0) << "%" << std::endl;
    }
    
    std::cout << "Current memory usage: " << detailed_stats.current_memory_usage << " bytes" << std::endl;
    std::cout << "Peak memory usage: " << detailed_stats.peak_memory_usage << " bytes" << std::endl;
    std::cout << "Average allocation time: " << std::fixed << std::setprecision(3) 
              << detailed_stats.average_allocation_time_ms << " ms" << std::endl;
    
    // Verify stats are reasonable
    EXPECT_GT(detailed_stats.total_allocations, 0);
    EXPECT_GE(detailed_stats.pool_hits + detailed_stats.pool_misses, detailed_stats.total_allocations);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
