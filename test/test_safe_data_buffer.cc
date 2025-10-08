#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "unilink/common/safe_data_buffer.hpp"

using namespace unilink;
using namespace unilink::common;

class SafeDataBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test data
        test_string_ = "Hello, World!";
        test_vector_ = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21};
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
    
    std::string test_string_;
    std::vector<uint8_t> test_vector_;
};

/**
 * @brief Test SafeDataBuffer construction from string
 */
TEST_F(SafeDataBufferTest, ConstructionFromString) {
    SafeDataBuffer buffer(test_string_);
    
    EXPECT_EQ(buffer.size(), test_string_.size());
    EXPECT_FALSE(buffer.empty());
    EXPECT_TRUE(buffer.is_valid());
    
    std::string result = buffer.as_string();
    EXPECT_EQ(result, test_string_);
}

/**
 * @brief Test SafeDataBuffer construction from vector
 */
TEST_F(SafeDataBufferTest, ConstructionFromVector) {
    SafeDataBuffer buffer(test_vector_);
    
    EXPECT_EQ(buffer.size(), test_vector_.size());
    EXPECT_FALSE(buffer.empty());
    
    // Test data access
    EXPECT_EQ(buffer.size(), test_vector_.size());
    
    for (size_t i = 0; i < test_vector_.size(); ++i) {
        EXPECT_EQ(buffer[i], test_vector_[i]);
    }
}

/**
 * @brief Test SafeDataBuffer construction from raw data
 */
TEST_F(SafeDataBufferTest, ConstructionFromRawData) {
    SafeDataBuffer buffer(test_vector_.data(), test_vector_.size());
    
    EXPECT_EQ(buffer.size(), test_vector_.size());
    EXPECT_FALSE(buffer.empty());
    
    for (size_t i = 0; i < test_vector_.size(); ++i) {
        EXPECT_EQ(buffer[i], test_vector_[i]);
    }
}


/**
 * @brief Test SafeDataBuffer bounds checking
 */
TEST_F(SafeDataBufferTest, BoundsChecking) {
    SafeDataBuffer buffer(test_string_);
    
    // Valid access
    EXPECT_NO_THROW(buffer[0]);
    EXPECT_NO_THROW(buffer.at(0));
    EXPECT_NO_THROW(buffer[buffer.size() - 1]);
    EXPECT_NO_THROW(buffer.at(buffer.size() - 1));
    
    // Invalid access
    EXPECT_THROW(buffer[buffer.size()], std::out_of_range);
    EXPECT_THROW(buffer.at(buffer.size()), std::out_of_range);
    EXPECT_THROW(buffer[buffer.size() + 1], std::out_of_range);
    EXPECT_THROW(buffer.at(buffer.size() + 1), std::out_of_range);
}

/**
 * @brief Test SafeDataBuffer comparison
 */
TEST_F(SafeDataBufferTest, Comparison) {
    SafeDataBuffer buffer1(test_string_);
    SafeDataBuffer buffer2(test_string_);
    SafeDataBuffer buffer3(std::string("Different string"));
    
    EXPECT_EQ(buffer1, buffer2);
    EXPECT_NE(buffer1, buffer3);
    EXPECT_NE(buffer2, buffer3);
}

/**
 * @brief Test SafeDataBuffer utility methods
 */
TEST_F(SafeDataBufferTest, UtilityMethods) {
    SafeDataBuffer buffer(test_string_);
    
    // Test clear
    EXPECT_FALSE(buffer.empty());
    buffer.clear();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    
    // Test reserve and resize
    buffer.reserve(100);
    buffer.resize(50);
    EXPECT_EQ(buffer.size(), 50);
}

/**
 * @brief Test SafeDataBuffer with empty data
 */
TEST_F(SafeDataBufferTest, EmptyData) {
    SafeDataBuffer buffer1(std::string(""));
    SafeDataBuffer buffer2(std::vector<uint8_t>{});
    SafeDataBuffer buffer3(static_cast<const uint8_t*>(nullptr), 0);
    
    EXPECT_TRUE(buffer1.empty());
    EXPECT_TRUE(buffer2.empty());
    EXPECT_TRUE(buffer3.empty());
    
    EXPECT_EQ(buffer1.size(), 0);
    EXPECT_EQ(buffer2.size(), 0);
    EXPECT_EQ(buffer3.size(), 0);
}

/**
 * @brief Test SafeDataBuffer factory functions
 */
TEST_F(SafeDataBufferTest, FactoryFunctions) {
    // Test from_string
    auto buffer1 = safe_buffer_factory::from_string(test_string_);
    EXPECT_EQ(buffer1.as_string(), test_string_);
    
    // Test from_c_string
    auto buffer2 = safe_buffer_factory::from_c_string(test_string_.c_str());
    EXPECT_EQ(buffer2.as_string(), test_string_);
    
    // Test from_vector
    auto buffer3 = safe_buffer_factory::from_vector(test_vector_);
    EXPECT_EQ(buffer3.size(), test_vector_.size());
    
    // Test from_raw_data
    auto buffer4 = safe_buffer_factory::from_raw_data(test_vector_.data(), test_vector_.size());
    EXPECT_EQ(buffer4.size(), test_vector_.size());
    
    // Test from_span
    ConstByteSpan span(test_vector_);
    auto buffer5 = safe_buffer_factory::from_span(span);
    EXPECT_EQ(buffer5.size(), test_vector_.size());
}

/**
 * @brief Test SafeDataBuffer with null pointer
 */
TEST_F(SafeDataBufferTest, NullPointerHandling) {
    // Should not throw with null pointer and zero size
    EXPECT_NO_THROW(SafeDataBuffer(static_cast<const uint8_t*>(nullptr), 0));
    EXPECT_NO_THROW(safe_buffer_factory::from_c_string(nullptr));
    
    // Should throw with null pointer and non-zero size
    EXPECT_THROW(SafeDataBuffer(static_cast<const uint8_t*>(nullptr), 10), std::invalid_argument);
}

/**
 * @brief Test SafeDataBuffer copy and move semantics
 */
TEST_F(SafeDataBufferTest, CopyAndMoveSemantics) {
    SafeDataBuffer original(test_string_);
    
    // Test copy constructor
    SafeDataBuffer copy(original);
    EXPECT_EQ(copy, original);
    EXPECT_EQ(copy.size(), original.size());
    
    // Test copy assignment
    SafeDataBuffer copy_assigned = original;
    EXPECT_EQ(copy_assigned, original);
    
    // Test move constructor
    SafeDataBuffer moved(std::move(copy));
    EXPECT_EQ(moved, original);
    EXPECT_TRUE(copy.empty()); // Moved-from object should be empty
    
    // Test move assignment
    SafeDataBuffer move_assigned = std::move(moved);
    EXPECT_EQ(move_assigned, original);
    EXPECT_TRUE(moved.empty()); // Moved-from object should be empty
}

/**
 * @brief Test SafeDataBuffer with SafeSpan
 */
TEST_F(SafeDataBufferTest, SafeSpanSupport) {
    // Test construction from span
    ConstByteSpan span(test_vector_);
    SafeDataBuffer buffer(span);
    
    EXPECT_EQ(buffer.size(), test_vector_.size());
    EXPECT_FALSE(buffer.empty());
    
    // Test as_span method
    auto result_span = buffer.as_span();
    EXPECT_EQ(result_span.size(), test_vector_.size());
    
    for (size_t i = 0; i < test_vector_.size(); ++i) {
        EXPECT_EQ(result_span[i], test_vector_[i]);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
