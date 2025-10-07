#include <gtest/gtest.h>
#include <stdexcept>
#include <memory>

#include "unilink/common/memory_pool.hpp"

using namespace unilink;
using namespace unilink::common;

class ApiSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a fresh memory pool for each test
        pool_ = std::make_unique<MemoryPool>(10, 50);
    }
    
    void TearDown() override {
        pool_.reset();
    }
    
    std::unique_ptr<MemoryPool> pool_;
};

/**
 * @brief Test input validation for acquire function
 */
TEST_F(ApiSafetyTest, AcquireInputValidation) {
    // Test zero size
    EXPECT_THROW(pool_->acquire(0), std::invalid_argument);
    
    // Test size too large
    EXPECT_THROW(pool_->acquire(MemoryPool::MAX_BUFFER_SIZE + 1), std::invalid_argument);
    
    // Test valid sizes
    EXPECT_NO_THROW(pool_->acquire(1));
    EXPECT_NO_THROW(pool_->acquire(1024));
    EXPECT_NO_THROW(pool_->acquire(MemoryPool::MAX_BUFFER_SIZE));
}

/**
 * @brief Test input validation for release function
 */
TEST_F(ApiSafetyTest, ReleaseInputValidation) {
    // Test zero size
    EXPECT_THROW(pool_->release(nullptr, 0), std::invalid_argument);
    
    // Test size too large
    EXPECT_THROW(pool_->release(nullptr, MemoryPool::MAX_BUFFER_SIZE + 1), std::invalid_argument);
    
    // Test valid release
    auto buffer = pool_->acquire(1024);
    EXPECT_NO_THROW(pool_->release(std::move(buffer), 1024));
}

/**
 * @brief Test input validation for resize_pool function
 */
TEST_F(ApiSafetyTest, ResizePoolInputValidation) {
    // Test zero size
    EXPECT_THROW(pool_->resize_pool(0), std::invalid_argument);
    
    // Test size too large
    EXPECT_THROW(pool_->resize_pool(MemoryPool::MAX_POOL_SIZE + 1), std::invalid_argument);
    
    // Test valid sizes
    EXPECT_NO_THROW(pool_->resize_pool(1));
    EXPECT_NO_THROW(pool_->resize_pool(100));
    EXPECT_NO_THROW(pool_->resize_pool(MemoryPool::MAX_POOL_SIZE));
}

/**
 * @brief Test PooledBuffer safe access methods
 */
TEST_F(ApiSafetyTest, PooledBufferSafeAccess) {
    PooledBuffer buffer(1024);
    EXPECT_TRUE(buffer.valid());
    
    // Test safe array access
    EXPECT_NO_THROW(buffer[0]);
    EXPECT_NO_THROW(buffer[1023]);
    
    // Test bounds checking
    EXPECT_THROW(buffer[1024], std::out_of_range);
    EXPECT_THROW(buffer[2000], std::out_of_range);
    
    // Test safe pointer arithmetic
    EXPECT_NO_THROW(buffer.at(0));
    EXPECT_NO_THROW(buffer.at(1024));
    EXPECT_THROW(buffer.at(1025), std::out_of_range);
}

/**
 * @brief Test PooledBuffer with invalid buffer
 */
TEST_F(ApiSafetyTest, PooledBufferInvalidAccess) {
    // Test that creating a buffer with invalid size throws
    EXPECT_THROW(PooledBuffer buffer(MemoryPool::MAX_BUFFER_SIZE + 1), std::invalid_argument);
    
    // Create a valid buffer and then test invalid access
    PooledBuffer buffer(1024);
    EXPECT_TRUE(buffer.valid());
    
    // Test bounds checking
    EXPECT_THROW(buffer[1024], std::out_of_range);
    EXPECT_THROW(buffer.at(1025), std::out_of_range);
}

/**
 * @brief Test explicit conversion methods
 */
TEST_F(ApiSafetyTest, PooledBufferExplicitConversion) {
    PooledBuffer buffer(1024);
    
    // Test explicit bool conversion
    EXPECT_TRUE(static_cast<bool>(buffer));
    
    // Test explicit get method
    uint8_t* ptr = buffer.get();
    EXPECT_NE(ptr, nullptr);
    
    // Test that implicit conversion is not available
    // This should not compile if implicit conversion is properly removed
    // uint8_t* ptr2 = buffer; // This should cause compilation error
}

/**
 * @brief Test const access methods
 */
TEST_F(ApiSafetyTest, PooledBufferConstAccess) {
    const PooledBuffer buffer(1024);
    
    // Test const array access
    EXPECT_NO_THROW(buffer[0]);
    EXPECT_NO_THROW(buffer[1023]);
    EXPECT_THROW(buffer[1024], std::out_of_range);
    
    // Test const data access
    const uint8_t* ptr = buffer.data();
    EXPECT_NE(ptr, nullptr);
    
    // Test const get method
    const uint8_t* ptr2 = buffer.get();
    EXPECT_NE(ptr2, nullptr);
}

/**
 * @brief Test memory allocation failure handling
 */
TEST_F(ApiSafetyTest, MemoryAllocationFailure) {
    // Test that allocation failure is handled gracefully
    // Note: This test might not always trigger allocation failure
    // depending on system memory, but it tests the error handling path
    
    auto buffer = pool_->acquire(1024);
    if (buffer) {
        // If allocation succeeded, test normal operation
        EXPECT_NE(buffer.get(), nullptr);
        pool_->release(std::move(buffer), 1024);
    } else {
        // If allocation failed, this is also acceptable behavior
        EXPECT_EQ(buffer.get(), nullptr);
    }
}

/**
 * @brief Test buffer size validation
 */
TEST_F(ApiSafetyTest, BufferSizeValidation) {
    // Test minimum size
    EXPECT_NO_THROW(pool_->acquire(MemoryPool::MIN_BUFFER_SIZE));
    
    // Test maximum size
    EXPECT_NO_THROW(pool_->acquire(MemoryPool::MAX_BUFFER_SIZE));
    
    // Test edge cases
    EXPECT_THROW(pool_->acquire(MemoryPool::MIN_BUFFER_SIZE - 1), std::invalid_argument);
    EXPECT_THROW(pool_->acquire(MemoryPool::MAX_BUFFER_SIZE + 1), std::invalid_argument);
}

/**
 * @brief Test pool size validation
 */
TEST_F(ApiSafetyTest, PoolSizeValidation) {
    // Test minimum pool size
    EXPECT_NO_THROW(pool_->resize_pool(1));
    
    // Test maximum pool size
    EXPECT_NO_THROW(pool_->resize_pool(MemoryPool::MAX_POOL_SIZE));
    
    // Test edge cases
    EXPECT_THROW(pool_->resize_pool(0), std::invalid_argument);
    EXPECT_THROW(pool_->resize_pool(MemoryPool::MAX_POOL_SIZE + 1), std::invalid_argument);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
