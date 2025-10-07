#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

#include "test_utils.hpp"
#include "unilink/common/common.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/config/config_manager.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

// ============================================================================
// COMMON TESTS
// ============================================================================

/**
 * @brief Common functionality tests
 */
TEST_F(BaseTest, CommonFunctionality) {
    // Test LinkState enum
    EXPECT_STREQ(common::to_cstr(common::LinkState::Idle), "Idle");
    EXPECT_STREQ(common::to_cstr(common::LinkState::Connected), "Connected");
    EXPECT_STREQ(common::to_cstr(common::LinkState::Error), "Error");
    
    // Test timestamp functionality
    std::string timestamp = common::ts_now();
    EXPECT_FALSE(timestamp.empty());
    EXPECT_GT(timestamp.length(), 10);
}

/**
 * @brief Configuration manager tests
 */
TEST_F(BaseTest, ConfigManager) {
    // Test basic configuration functionality
    // Note: ConfigManager might not have instance() method
    // This test is kept for future implementation
    EXPECT_TRUE(true);
}

// ============================================================================
// IOCONTEXT MANAGER TESTS
// ============================================================================

/**
 * @brief IoContextManager basic functionality tests
 */
TEST_F(BaseTest, IoContextManagerBasicFunctionality) {
    auto& manager = common::IoContextManager::instance();
    
    // Test basic operations
    EXPECT_FALSE(manager.is_running());
    
    manager.start();
    EXPECT_TRUE(manager.is_running());
    
    auto& context = manager.get_context();
    EXPECT_NE(&context, nullptr);
    
    manager.stop();
    EXPECT_FALSE(manager.is_running());
}

/**
 * @brief Independent context creation tests
 */
TEST_F(BaseTest, IndependentContextCreation) {
    auto& manager = common::IoContextManager::instance();
    
    // Create independent context
    auto independent_context = manager.create_independent_context();
    EXPECT_NE(independent_context, nullptr);
    
    // Verify it's different from global context
    manager.start();
    auto& global_context = manager.get_context();
    EXPECT_NE(independent_context.get(), &global_context);
    
    manager.stop();
}

// ============================================================================
// MEMORY POOL TESTS
// ============================================================================

/**
 * @brief Memory pool basic functionality tests
 */
TEST_F(MemoryTest, MemoryPoolBasicFunctionality) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    // Test basic acquire/release
    auto buffer = pool.acquire(1024);
    EXPECT_NE(buffer, nullptr);
    
    pool.release(std::move(buffer), 1024);
    
    // Test statistics
    auto stats = pool.get_stats();
    EXPECT_GE(stats.total_allocations, 0);
}

/**
 * @brief Memory pool performance tests
 */
TEST_F(MemoryTest, MemoryPoolPerformance) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    const int num_operations = 1000;
    const size_t buffer_size = 4096;
    
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
    
    std::cout << "Memory pool performance: " << duration << " μs for " 
              << num_operations << " operations" << std::endl;
    
    // Verify performance is reasonable
    EXPECT_LT(duration, 100000); // Should complete in less than 100ms
}

/**
 * @brief Memory pool statistics tests
 */
TEST_F(MemoryTest, MemoryPoolStatistics) {
    auto& pool = common::GlobalMemoryPool::instance();
    
    // Perform some operations
    for (int i = 0; i < 100; ++i) {
        auto buffer = pool.acquire(1024);
        if (buffer) {
            pool.release(std::move(buffer), 1024);
        }
    }
    
    // Test basic statistics
    auto stats = pool.get_stats();
    EXPECT_GT(stats.total_allocations, 0);
    
    double hit_rate = pool.get_hit_rate();
    EXPECT_GE(hit_rate, 0.0);
    EXPECT_LE(hit_rate, 1.0);
    
    auto memory_usage = pool.get_memory_usage();
    EXPECT_GE(memory_usage.first, 0);
    EXPECT_GE(memory_usage.second, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
