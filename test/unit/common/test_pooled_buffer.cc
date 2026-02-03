/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>

#include "unilink/memory/memory_pool.hpp"

using namespace unilink::memory;

class PooledBufferTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Ensure clean state
    auto& pool = GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));
  }
};

TEST_F(PooledBufferTest, ConstructionAndValidity) {
  PooledBuffer buffer(1024);
  EXPECT_TRUE(buffer.valid());
  EXPECT_EQ(buffer.size(), 1024);
  EXPECT_NE(buffer.data(), nullptr);
}

TEST_F(PooledBufferTest, OperatorSquareBrackets) {
  const size_t size = 100;
  PooledBuffer buffer(size);
  ASSERT_TRUE(buffer.valid());

  // Write
  for (size_t i = 0; i < size; ++i) {
    buffer[i] = static_cast<uint8_t>(i);
  }

  // Read
  for (size_t i = 0; i < size; ++i) {
    EXPECT_EQ(buffer[i], static_cast<uint8_t>(i));
  }

  // Const access
  const PooledBuffer& const_buffer = buffer;
  for (size_t i = 0; i < size; ++i) {
    EXPECT_EQ(const_buffer[i], static_cast<uint8_t>(i));
  }
}

TEST_F(PooledBufferTest, AtMethodValidAccess) {
  const size_t size = 100;
  PooledBuffer buffer(size);
  ASSERT_TRUE(buffer.valid());

  // Write via []
  for (size_t i = 0; i < size; ++i) {
    buffer[i] = static_cast<uint8_t>(i);
  }

  // Read via at()
  for (size_t i = 0; i < size; ++i) {
    uint8_t* ptr = buffer.at(i);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(*ptr, static_cast<uint8_t>(i));
  }
}

TEST_F(PooledBufferTest, AtMethodOutOfBounds) {
  const size_t size = 100;
  PooledBuffer buffer(size);
  ASSERT_TRUE(buffer.valid());

  EXPECT_THROW(buffer.at(size), std::out_of_range);
  EXPECT_THROW(buffer.at(size + 1), std::out_of_range);
}
