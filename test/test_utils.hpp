#pragma once

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "unilink/common/memory_pool.hpp"

namespace unilink {
namespace test {

/**
 * @brief Common test utilities for unilink tests
 */
class TestUtils {
public:
    /**
     * @brief Get a unique test port number
     * @return uint16_t Unique port number
     */
    static uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{20000};
        return port_counter.fetch_add(1);
    }
    
    /**
     * @brief Wait for a condition with timeout
     * @param condition Function that returns true when condition is met
     * @param timeout_ms Timeout in milliseconds
     * @return true if condition was met, false if timeout
     */
    template<typename Condition>
    static bool waitForCondition(Condition&& condition, int timeout_ms = 5000) {
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(timeout_ms);
        
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (condition()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }
    
    /**
     * @brief Wait for a specific duration
     * @param ms Duration in milliseconds
     */
    static void waitFor(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    
    /**
     * @brief Generate test data of specified size
     * @param size Size of test data
     * @return std::string Test data
     */
    static std::string generateTestData(size_t size) {
        std::string data;
        data.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            data += static_cast<char>('A' + (i % 26));
        }
        return data;
    }
};

/**
 * @brief Base test class with common setup/teardown
 */
class BaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
        test_start_time_ = std::chrono::steady_clock::now();
    }
    
    void TearDown() override {
        // Common teardown for all tests
        auto test_duration = std::chrono::steady_clock::now() - test_start_time_;
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_duration).count();
        
        // Log test duration if it's unusually long
        if (duration_ms > 5000) {
            std::cout << "Warning: Test took " << duration_ms << "ms to complete" << std::endl;
        }
    }
    
    std::chrono::steady_clock::time_point test_start_time_;
};

/**
 * @brief Test class for network-related tests
 */
class NetworkTest : public BaseTest {
protected:
    void SetUp() override {
        BaseTest::SetUp();
        test_port_ = TestUtils::getTestPort();
    }
    
    void TearDown() override {
        // Clean up any network resources
        BaseTest::TearDown();
    }
    
    uint16_t test_port_;
};

/**
 * @brief Test class for performance tests
 */
class PerformanceTest : public BaseTest {
protected:
    void SetUp() override {
        BaseTest::SetUp();
        performance_start_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        auto performance_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            performance_end - performance_start_).count();
        
        std::cout << "Performance test completed in " << duration << " μs" << std::endl;
        BaseTest::TearDown();
    }
    
    std::chrono::high_resolution_clock::time_point performance_start_;
};

/**
 * @brief Test class for memory-related tests
 */
class MemoryTest : public BaseTest {
protected:
    void SetUp() override {
        BaseTest::SetUp();
        // Reset memory pool for clean testing
        auto& pool = unilink::common::GlobalMemoryPool::instance();
        pool.cleanup_old_buffers(std::chrono::milliseconds(0));
    }
    
    void TearDown() override {
        // Clean up memory pool
        auto& pool = unilink::common::GlobalMemoryPool::instance();
        pool.cleanup_old_buffers(std::chrono::milliseconds(0));
        BaseTest::TearDown();
    }
};

/**
 * @brief Test class for integration tests
 */
class IntegrationTest : public NetworkTest {
protected:
    void SetUp() override {
        NetworkTest::SetUp();
        // Additional setup for integration tests
    }
    
    void TearDown() override {
        // Clean up integration test resources
        NetworkTest::TearDown();
    }
};

} // namespace test
} // namespace unilink
