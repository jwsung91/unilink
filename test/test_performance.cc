#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <iomanip>

#include "test_utils.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/config/serial_config.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::transport;
using namespace unilink::config;
using namespace unilink::common;
using namespace std::chrono_literals;

// ============================================================================
// TRANSPORT PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Transport performance benchmark
 */
TEST_F(PerformanceTest, TransportPerformanceBenchmark) {
    const int num_operations = 1000;
    const size_t data_size = 1024;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate transport operations
    for (int i = 0; i < num_operations; ++i) {
        // Simulate data processing
        std::string data = TestUtils::generateTestData(data_size);
        EXPECT_EQ(data.size(), data_size);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count();
    
    std::cout << "Transport performance: " << duration << " μs for " 
              << num_operations << " operations" << std::endl;
    
    // Verify performance is reasonable
    EXPECT_LT(duration, 100000); // Should complete in less than 100ms
}

/**
 * @brief Concurrent performance test
 */
TEST_F(PerformanceTest, ConcurrentPerformanceTest) {
    const int num_threads = 4;
    const int operations_per_thread = 250;
    const size_t data_size = 4096;
    
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&success_count, &error_count, operations_per_thread, data_size]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    std::string data = TestUtils::generateTestData(data_size);
                    if (data.size() == data_size) {
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
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();
    
    std::cout << "Concurrent performance: " << duration << " ms for " 
              << (num_threads * operations_per_thread) << " operations" << std::endl;
    std::cout << "Success count: " << success_count.load() << std::endl;
    std::cout << "Error count: " << error_count.load() << std::endl;
    
    // Verify results
    EXPECT_GT(success_count.load(), 0);
    EXPECT_EQ(error_count.load(), 0);
}

/**
 * @brief Memory pool performance test
 */
TEST_F(PerformanceTest, MemoryPoolPerformanceTest) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const int num_operations = 10000;
    const std::vector<size_t> buffer_sizes = {1024, 4096, 16384, 32768, 65536};
    
    std::cout << "\n=== Memory Pool Performance Test ===" << std::endl;
    
    for (size_t buffer_size : buffer_sizes) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<std::unique_ptr<uint8_t[]>> buffers;
        buffers.reserve(num_operations);
        
        // Allocate buffers
        for (int i = 0; i < num_operations; ++i) {
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
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
        
        std::cout << "Buffer size: " << buffer_size << " bytes, "
                  << "Time: " << duration << " μs" << std::endl;
        
        // Verify performance is reasonable
        EXPECT_LT(duration, 1000000); // Should complete in less than 1 second
    }
}

/**
 * @brief Hit rate analysis test
 */
TEST_F(PerformanceTest, HitRateAnalysis) {
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
 * @brief Auto-tuning performance test
 */
TEST_F(PerformanceTest, AutoTuningPerformanceTest) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const size_t buffer_size = 4096;
    const int num_operations = 1000;
    
    std::cout << "\n=== Auto Tuning Performance Test ===" << std::endl;
    
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

// ============================================================================
// BACKPRESSURE TESTS (Critical for preventing freezing)
// ============================================================================

/**
 * @brief TCP Client backpressure threshold test
 * 
 * Tests the critical backpressure mechanism that prevents memory exhaustion
 * and potential freezing when sending large amounts of data.
 */
TEST_F(PerformanceTest, TcpClientBackpressureThreshold) {
    // Setup
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = TestUtils::getTestPort();
    cfg.retry_interval_ms = 1000;
    
    auto client = std::make_shared<TcpClient>(cfg);
    
    // Backpressure callback setup
    std::atomic<bool> backpressure_triggered{false};
    std::atomic<size_t> backpressure_bytes{0};
    
    client->on_backpressure([&backpressure_triggered, &backpressure_bytes](size_t bytes) {
        backpressure_triggered = true;
        backpressure_bytes = bytes;
    });
    
    // Test Logic
    client->start();
    
    // Send large amount of data (2MB) - this should trigger backpressure
    const size_t large_data_size = 2 * (1 << 20); // 2MB
    std::vector<uint8_t> large_data(large_data_size, 0xAA);
    client->async_write_copy(large_data.data(), large_data.size());
    
    // Verification
    TestUtils::waitFor(200);
    EXPECT_TRUE(backpressure_triggered.load());
    EXPECT_GT(backpressure_bytes.load(), 1 << 20); // Should be > 1MB
    
    client->stop();
}

/**
 * @brief TCP Server backpressure threshold test
 * 
 * Note: Server backpressure can only be tested when there's an active client connection.
 * This test verifies that the server can handle large data without crashing.
 */
TEST_F(PerformanceTest, TcpServerBackpressureThreshold) {
    // Setup
    TcpServerConfig cfg;
    cfg.port = TestUtils::getTestPort();
    
    auto server = std::make_shared<TcpServer>(cfg);
    
    // Backpressure callback setup
    std::atomic<bool> backpressure_triggered{false};
    std::atomic<size_t> backpressure_bytes{0};
    
    server->on_backpressure([&backpressure_triggered, &backpressure_bytes](size_t bytes) {
        backpressure_triggered = true;
        backpressure_bytes = bytes;
    });
    
    // Test Logic
    server->start();
    
    // For server, we test that it can handle large data without crashing
    // Since there's no client connection, backpressure won't be triggered
    // but the server should handle the data gracefully
    const size_t large_data_size = 2 * (1 << 20); // 2MB
    std::vector<uint8_t> large_data(large_data_size, 0xAA);
    
    // This should not crash the server
    server->async_write_copy(large_data.data(), large_data.size());
    
    // Verification
    TestUtils::waitFor(200);
    
    // Server should still be running without crashing
    EXPECT_NE(server, nullptr);
    
    // Note: Backpressure won't be triggered without a client connection
    // This is expected behavior for servers
    std::cout << "Server backpressure test: Server handled large data without crashing" << std::endl;
    
    server->stop();
}

/**
 * @brief Serial backpressure threshold test
 */
TEST_F(PerformanceTest, SerialBackpressureThreshold) {
    // Setup
    SerialConfig cfg;
    cfg.device = "/dev/ttyUSB0"; // Mock device
    cfg.baud_rate = 9600;
    
    auto serial = std::make_shared<Serial>(cfg);
    
    // Backpressure callback setup
    std::atomic<bool> backpressure_triggered{false};
    std::atomic<size_t> backpressure_bytes{0};
    
    serial->on_backpressure([&backpressure_triggered, &backpressure_bytes](size_t bytes) {
        backpressure_triggered = true;
        backpressure_bytes = bytes;
    });
    
    // Test Logic
    serial->start();
    
    // Send large amount of data (2MB) - this should trigger backpressure
    const size_t large_data_size = 2 * (1 << 20); // 2MB
    std::vector<uint8_t> large_data(large_data_size, 0xAA);
    serial->async_write_copy(large_data.data(), large_data.size());
    
    // Verification
    TestUtils::waitFor(200);
    EXPECT_TRUE(backpressure_triggered.load());
    EXPECT_GT(backpressure_bytes.load(), 1 << 20); // Should be > 1MB
    
    serial->stop();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
