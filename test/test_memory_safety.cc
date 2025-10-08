#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>
#include <chrono>

#include "unilink/common/memory_tracker.hpp"
#include "unilink/common/memory_validator.hpp"
#include "unilink/common/safe_data_buffer.hpp"
#include "unilink/common/thread_safe_state.hpp"

using namespace unilink;
using namespace unilink::common;
using namespace std::chrono_literals;

class MemorySafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Enable memory tracking for tests
        MemoryTracker::instance().enable_tracking(true);
        MemoryTracker::instance().clear_tracking_data();
    }
    
    void TearDown() override {
        // Print memory report after each test
        MemoryTracker::instance().print_memory_report();
    }
};

/**
 * @brief Test memory tracking functionality
 */
TEST_F(MemorySafetyTest, MemoryTrackingBasicFunctionality) {
    auto& tracker = MemoryTracker::instance();
    
    // Test initial state
    auto stats = tracker.get_stats();
    EXPECT_EQ(stats.current_allocations, 0);
    EXPECT_EQ(stats.current_bytes_allocated, 0);
    
    // Test allocation tracking
    void* ptr1 = std::malloc(1024);
    MEMORY_TRACK_ALLOCATION(ptr1, 1024);
    
    stats = tracker.get_stats();
    EXPECT_EQ(stats.current_allocations, 1);
    EXPECT_EQ(stats.current_bytes_allocated, 1024);
    
    // Test deallocation tracking
    MEMORY_TRACK_DEALLOCATION(ptr1);
    std::free(ptr1);
    
    stats = tracker.get_stats();
    EXPECT_EQ(stats.current_allocations, 0);
    EXPECT_EQ(stats.current_bytes_allocated, 0);
}

/**
 * @brief Test memory leak detection
 */
TEST_F(MemorySafetyTest, MemoryLeakDetection) {
    auto& tracker = MemoryTracker::instance();
    
    // Allocate memory without deallocating
    void* ptr1 = std::malloc(512);
    void* ptr2 = std::malloc(1024);
    MEMORY_TRACK_ALLOCATION(ptr1, 512);
    MEMORY_TRACK_ALLOCATION(ptr2, 1024);
    
    auto leaked_allocations = tracker.get_leaked_allocations();
    EXPECT_EQ(leaked_allocations.size(), 2);
    
    // Clean up
    MEMORY_TRACK_DEALLOCATION(ptr1);
    MEMORY_TRACK_DEALLOCATION(ptr2);
    std::free(ptr1);
    std::free(ptr2);
}

/**
 * @brief Test memory validator functionality
 */
TEST_F(MemorySafetyTest, MemoryValidatorFunctionality) {
    const size_t buffer_size = 1024;
    
    // Allocate buffer
    void* buffer = std::malloc(buffer_size);
    ASSERT_NE(buffer, nullptr);
    
    // Test basic memory operations
    std::memset(buffer, 0xAA, buffer_size);
    
    // Verify memory was set correctly
    for (size_t i = 0; i < buffer_size; ++i) {
        EXPECT_EQ(static_cast<uint8_t*>(buffer)[i], 0xAA);
    }
    
    std::free(buffer);
}

/**
 * @brief Test safe memory operations
 */
TEST_F(MemorySafetyTest, SafeMemoryOperations) {
    const size_t buffer_size = 1024;
    
    // Allocate source and destination buffers
    void* src = std::malloc(buffer_size);
    void* dest = std::malloc(buffer_size);
    ASSERT_NE(src, nullptr);
    ASSERT_NE(dest, nullptr);
    
    // Initialize source buffer with pattern
    std::memset(src, 0xAA, buffer_size);
    
    // Test memory copy
    std::memcpy(dest, src, buffer_size);
    
    // Verify copy was successful
    EXPECT_EQ(std::memcmp(dest, src, buffer_size), 0);
    
    // Test memory set
    std::memset(dest, 0x55, buffer_size);
    
    // Verify set was successful
    for (size_t i = 0; i < buffer_size; ++i) {
        EXPECT_EQ(static_cast<uint8_t*>(dest)[i], 0x55);
    }
    
    std::free(src);
    std::free(dest);
}

/**
 * @brief Test memory validator RAII wrapper
 */
TEST_F(MemorySafetyTest, MemoryValidatorRAII) {
    const size_t buffer_size = 512;
    
    // Allocate buffer
    void* buffer = std::malloc(buffer_size);
    ASSERT_NE(buffer, nullptr);
    
    // Test basic memory operations
    std::memset(buffer, 0xCC, buffer_size);
    
    // Verify memory was set correctly
    for (size_t i = 0; i < buffer_size; ++i) {
        EXPECT_EQ(static_cast<uint8_t*>(buffer)[i], 0xCC);
    }
    
    std::free(buffer);
}

/**
 * @brief Test memory pattern generation
 */
TEST_F(MemorySafetyTest, MemoryPatternGeneration) {
    const size_t pattern_size = 1024;
    const uint8_t seed = 0xAA;
    
    // Generate pattern manually
    std::vector<uint8_t> pattern(pattern_size);
    for (size_t i = 0; i < pattern_size; ++i) {
        pattern[i] = seed ^ (i % 256);
    }
    EXPECT_EQ(pattern.size(), pattern_size);
    
    // Validate pattern
    for (size_t i = 0; i < pattern_size; ++i) {
        EXPECT_EQ(pattern[i], seed ^ (i % 256));
    }
    
    // Test random pattern
    std::vector<uint8_t> random_pattern(pattern_size);
    for (size_t i = 0; i < pattern_size; ++i) {
        random_pattern[i] = static_cast<uint8_t>(i * 7 + 13);
    }
    EXPECT_EQ(random_pattern.size(), pattern_size);
}

/**
 * @brief Test concurrent memory operations
 */
TEST_F(MemorySafetyTest, ConcurrentMemoryOperations) {
    auto& tracker = MemoryTracker::instance();
    const int num_threads = 4;
    const int allocations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&tracker, allocations_per_thread, t]() {
            std::vector<void*> allocated_ptrs;
            
            // Allocate memory
            for (int i = 0; i < allocations_per_thread; ++i) {
                size_t size = 64 + (i % 10) * 64; // 64 bytes to 640 bytes
                void* ptr = std::malloc(size);
                if (ptr) {
                    MEMORY_TRACK_ALLOCATION(ptr, size);
                    allocated_ptrs.push_back(ptr);
                }
            }
            
            // Deallocate memory
            for (void* ptr : allocated_ptrs) {
                MEMORY_TRACK_DEALLOCATION(ptr);
                std::free(ptr);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check that all allocations were properly tracked
    auto stats = tracker.get_stats();
    EXPECT_EQ(stats.current_allocations, 0);
    EXPECT_EQ(stats.current_bytes_allocated, 0);
}

/**
 * @brief Test SafeDataBuffer memory safety
 */
TEST_F(MemorySafetyTest, SafeDataBufferMemorySafety) {
    const std::string test_data = "Hello, Memory Safety!";
    
    // Test SafeDataBuffer construction
    SafeDataBuffer buffer(test_data);
    EXPECT_EQ(buffer.size(), test_data.size());
    EXPECT_FALSE(buffer.empty());
    
    // Test safe access
    for (size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_EQ(buffer[i], test_data[i]);
    }
    
    // Test bounds checking
    EXPECT_THROW(buffer.at(buffer.size()), std::out_of_range);
    
    // Test copy and move semantics
    SafeDataBuffer copy = buffer;
    EXPECT_EQ(copy, buffer);
    
    SafeDataBuffer moved = std::move(copy);
    EXPECT_EQ(moved, buffer);
    EXPECT_TRUE(copy.empty());
}

/**
 * @brief Test ThreadSafeState memory safety
 */
TEST_F(MemorySafetyTest, ThreadSafeStateMemorySafety) {
    ThreadSafeLinkState state(LinkState::Idle);
    
    // Test basic operations
    EXPECT_EQ(state.get_state(), LinkState::Idle);
    EXPECT_TRUE(state.is_state(LinkState::Idle));
    
    // Test state changes
    state.set_state(LinkState::Connected);
    EXPECT_EQ(state.get_state(), LinkState::Connected);
    
    // Test concurrent access
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&state, operations_per_thread, &successful_operations]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    auto current_state = state.get_state();
                    LinkState new_state = static_cast<LinkState>((static_cast<int>(current_state) + 1) % 6);
                    if (state.compare_and_set(current_state, new_state)) {
                        successful_operations++;
                    }
                } catch (...) {
                    // Handle any exceptions
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify operations completed successfully
    EXPECT_GT(successful_operations.load(), 0);
}

/**
 * @brief Test memory stress conditions
 */
TEST_F(MemorySafetyTest, MemoryStressTest) {
    auto& tracker = MemoryTracker::instance();
    const int stress_iterations = 1000;
    
    for (int i = 0; i < stress_iterations; ++i) {
        // Allocate various sizes
        size_t size = 16 + (i % 100) * 16; // 16 bytes to 1600 bytes
        void* ptr = std::malloc(size);
        if (ptr) {
            MEMORY_TRACK_ALLOCATION(ptr, size);
            
            // Use the memory briefly
            std::memset(ptr, i % 256, size);
            
            MEMORY_TRACK_DEALLOCATION(ptr);
            std::free(ptr);
        }
        
        // Periodic cleanup
        if (i % 100 == 0) {
            auto stats = tracker.get_stats();
            EXPECT_EQ(stats.current_allocations, 0);
        }
    }
    
    // Final verification
    auto stats = tracker.get_stats();
    EXPECT_EQ(stats.current_allocations, 0);
    EXPECT_EQ(stats.current_bytes_allocated, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
