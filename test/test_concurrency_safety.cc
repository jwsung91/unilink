#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>

#include "unilink/common/thread_safe_state.hpp"
#include "unilink/common/common.hpp"

using namespace unilink;
using namespace unilink::common;
using namespace std::chrono_literals;

class ConcurrencySafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }
    
    void TearDown() override {
        // Test cleanup
    }
};

/**
 * @brief Test ThreadSafeState basic functionality
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeStateBasicFunctionality) {
    ThreadSafeLinkState state(LinkState::Idle);
    
    // Test initial state
    EXPECT_EQ(state.get_state(), LinkState::Idle);
    EXPECT_TRUE(state.is_state(LinkState::Idle));
    
    // Test state change
    state.set_state(LinkState::Connected);
    EXPECT_EQ(state.get_state(), LinkState::Connected);
    EXPECT_TRUE(state.is_state(LinkState::Connected));
    
    // Test compare and set
    EXPECT_TRUE(state.compare_and_set(LinkState::Connected, LinkState::Closed));
    EXPECT_EQ(state.get_state(), LinkState::Closed);
    
    EXPECT_FALSE(state.compare_and_set(LinkState::Connected, LinkState::Idle));
    EXPECT_EQ(state.get_state(), LinkState::Closed);
}

/**
 * @brief Test ThreadSafeState concurrent access
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeStateConcurrentAccess) {
    ThreadSafeLinkState state(LinkState::Idle);
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    std::atomic<int> failed_operations{0};
    
    // Start multiple threads that read and write state
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&state, operations_per_thread, &successful_operations, &failed_operations, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    // Read state
                    auto current_state = state.get_state();
                    
                    // Try to change state
                    LinkState new_state = static_cast<LinkState>((static_cast<int>(current_state) + 1) % 6);
                    if (state.compare_and_set(current_state, new_state)) {
                        successful_operations++;
                    } else {
                        failed_operations++;
                    }
                    
                    // Small delay to increase chance of race conditions
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                } catch (...) {
                    failed_operations++;
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify results
    EXPECT_GT(successful_operations.load(), 0);
    EXPECT_GE(failed_operations.load(), 0);
    
    std::cout << "Successful operations: " << successful_operations.load() << std::endl;
    std::cout << "Failed operations: " << failed_operations.load() << std::endl;
}

/**
 * @brief Test ThreadSafeState callbacks
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeStateCallbacks) {
    ThreadSafeLinkState state(LinkState::Idle);
    std::atomic<int> callback_count{0};
    std::vector<LinkState> received_states;
    std::mutex states_mutex;
    
    // Add callback
    state.add_state_change_callback([&callback_count, &received_states, &states_mutex](const LinkState& new_state) {
        callback_count++;
        std::lock_guard<std::mutex> lock(states_mutex);
        received_states.push_back(new_state);
    });
    
    // Change state multiple times
    state.set_state(LinkState::Connecting);
    state.set_state(LinkState::Connected);
    state.set_state(LinkState::Closed);
    
    // Wait a bit for callbacks to be processed
    std::this_thread::sleep_for(10ms);
    
    // Verify callbacks were called
    EXPECT_EQ(callback_count.load(), 3);
    
    std::lock_guard<std::mutex> lock(states_mutex);
    EXPECT_EQ(received_states.size(), 3);
    EXPECT_EQ(received_states[0], LinkState::Connecting);
    EXPECT_EQ(received_states[1], LinkState::Connected);
    EXPECT_EQ(received_states[2], LinkState::Closed);
}

/**
 * @brief Test AtomicState functionality
 */
TEST_F(ConcurrencySafetyTest, AtomicStateFunctionality) {
    AtomicLinkState state(LinkState::Idle);
    
    // Test basic operations
    EXPECT_EQ(state.get(), LinkState::Idle);
    EXPECT_TRUE(state.is_state(LinkState::Idle));
    
    state.set(LinkState::Connected);
    EXPECT_EQ(state.get(), LinkState::Connected);
    
    // Test compare and set
    EXPECT_TRUE(state.compare_and_set(LinkState::Connected, LinkState::Closed));
    EXPECT_EQ(state.get(), LinkState::Closed);
    
    EXPECT_FALSE(state.compare_and_set(LinkState::Connected, LinkState::Idle));
    EXPECT_EQ(state.get(), LinkState::Closed);
    
    // Test exchange
    auto old_state = state.exchange(LinkState::Error);
    EXPECT_EQ(old_state, LinkState::Closed);
    EXPECT_EQ(state.get(), LinkState::Error);
}

/**
 * @brief Test ThreadSafeCounter functionality
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeCounterFunctionality) {
    ThreadSafeCounter counter(0);
    
    // Test basic operations
    EXPECT_EQ(counter.get(), 0);
    
    EXPECT_EQ(counter.increment(), 1);
    EXPECT_EQ(counter.get(), 1);
    
    EXPECT_EQ(counter.decrement(), 0);
    EXPECT_EQ(counter.get(), 0);
    
    EXPECT_EQ(counter.add(5), 5);
    EXPECT_EQ(counter.get(), 5);
    
    EXPECT_EQ(counter.subtract(2), 3);
    EXPECT_EQ(counter.get(), 3);
    
    // Test compare and set
    EXPECT_TRUE(counter.compare_and_set(3, 10));
    EXPECT_EQ(counter.get(), 10);
    
    EXPECT_FALSE(counter.compare_and_set(3, 15));
    EXPECT_EQ(counter.get(), 10);
    
    // Test exchange
    EXPECT_EQ(counter.exchange(20), 10);
    EXPECT_EQ(counter.get(), 20);
    
    // Test reset
    counter.reset();
    EXPECT_EQ(counter.get(), 0);
}

/**
 * @brief Test ThreadSafeCounter concurrent access
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeCounterConcurrentAccess) {
    ThreadSafeCounter counter(0);
    const int num_threads = 10;
    const int operations_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    // Start multiple threads that increment the counter
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&counter, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                counter.increment();
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify final count
    int64_t expected_count = num_threads * operations_per_thread;
    EXPECT_EQ(counter.get(), expected_count);
    
    std::cout << "Final counter value: " << counter.get() << std::endl;
    std::cout << "Expected value: " << expected_count << std::endl;
}

/**
 * @brief Test ThreadSafeFlag functionality
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeFlagFunctionality) {
    ThreadSafeFlag flag(false);
    
    // Test basic operations
    EXPECT_FALSE(flag.get());
    
    flag.set(true);
    EXPECT_TRUE(flag.get());
    
    flag.clear();
    EXPECT_FALSE(flag.get());
    
    // Test test_and_set
    EXPECT_FALSE(flag.test_and_set());
    EXPECT_TRUE(flag.get());
    
    EXPECT_TRUE(flag.test_and_set());
    EXPECT_TRUE(flag.get());
    
    // Test compare and set
    EXPECT_TRUE(flag.compare_and_set(true, false));
    EXPECT_FALSE(flag.get());
    
    EXPECT_FALSE(flag.compare_and_set(true, false));
    EXPECT_FALSE(flag.get());
}

/**
 * @brief Test ThreadSafeFlag wait functionality
 */
TEST_F(ConcurrencySafetyTest, ThreadSafeFlagWaitFunctionality) {
    ThreadSafeFlag flag(false);
    
    // Test wait for true
    std::atomic<bool> wait_completed{false};
    std::thread waiter([&flag, &wait_completed]() {
        flag.wait_for_true(100ms);
        wait_completed.store(true);
    });
    
    // Set flag after a short delay
    std::this_thread::sleep_for(10ms);
    flag.set(true);
    
    waiter.join();
    EXPECT_TRUE(wait_completed.load());
}

/**
 * @brief Test complex concurrent scenario
 */
TEST_F(ConcurrencySafetyTest, ComplexConcurrentScenario) {
    ThreadSafeLinkState state(LinkState::Idle);
    ThreadSafeCounter counter(0);
    ThreadSafeFlag flag(false);
    
    const int num_threads = 5;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&state, &counter, &flag, operations_per_thread, &successful_operations, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                try {
                    // Complex state management
                    auto current_state = state.get_state();
                    
                    if (current_state == LinkState::Idle) {
                        if (state.compare_and_set(LinkState::Idle, LinkState::Connecting)) {
                            counter.increment();
                            
                            // Simulate some work
                            std::this_thread::sleep_for(std::chrono::microseconds(10));
                            
                            if (state.compare_and_set(LinkState::Connecting, LinkState::Connected)) {
                                counter.increment();
                                flag.set(true);
                                successful_operations++;
                            }
                        }
                    }
                } catch (...) {
                    // Handle exceptions
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify results
    EXPECT_GT(successful_operations.load(), 0);
    EXPECT_TRUE(flag.get());
    
    std::cout << "Successful operations: " << successful_operations.load() << std::endl;
    std::cout << "Final counter value: " << counter.get() << std::endl;
    std::cout << "Final state: " << to_cstr(state.get_state()) << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
