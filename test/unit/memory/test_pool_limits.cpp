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

#include <vector>

#include "unilink/memory/memory_pool.hpp"

using namespace unilink::memory;

namespace {

class MemoryPoolLimitsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // max_pool_size = 4 implies capacity = 1 for each of the 4 buckets
    pool_ = std::make_unique<MemoryPool>(1, 4);
  }

  std::unique_ptr<MemoryPool> pool_;
};

TEST_F(MemoryPoolLimitsTest, ReuseLogic) {
  // 1. Allocate one item
  auto buf1 = pool_->acquire(MemoryPool::BufferSize::SMALL);
  uint8_t* addr1 = buf1.get();
  EXPECT_NE(addr1, nullptr);

  // 2. Release it (should go back to pool)
  pool_->release(std::move(buf1), static_cast<size_t>(MemoryPool::BufferSize::SMALL));

  // 3. Allocate again
  auto buf2 = pool_->acquire(MemoryPool::BufferSize::SMALL);
  uint8_t* addr2 = buf2.get();

  // 4. Verify address reuse
  EXPECT_EQ(addr1, addr2) << "Memory address should be reused from the pool";
}

TEST_F(MemoryPoolLimitsTest, ExpansionAndOverflow) {
  // Capacity is 1 per bucket.

  // 1. Allocate first item (from heap, or pool if pre-filled?)
  // Implementation doesn't pre-fill.
  auto buf1 = pool_->acquire(MemoryPool::BufferSize::SMALL);

  // 2. Allocate second item (should trigger expansion/new allocation)
  auto buf2 = pool_->acquire(MemoryPool::BufferSize::SMALL);

  EXPECT_NE(buf1.get(), nullptr);
  EXPECT_NE(buf2.get(), nullptr);
  EXPECT_NE(buf1.get(), buf2.get());

  // 3. Release both.
  // First release should fit in pool.
  // Second release should overflow and be deleted.
  uint8_t* addr1 = buf1.get();
  // uint8_t* addr2 = buf2.get();

  pool_->release(std::move(buf1), static_cast<size_t>(MemoryPool::BufferSize::SMALL));
  // Pool now has 1 item (addr1).

  pool_->release(std::move(buf2), static_cast<size_t>(MemoryPool::BufferSize::SMALL));
  // Pool still has 1 item (addr1). buf2 was dropped.

  // 4. Acquire again. Should get addr1.
  auto buf3 = pool_->acquire(MemoryPool::BufferSize::SMALL);
  EXPECT_EQ(buf3.get(), addr1) << "Should get the pooled item";

  // 5. Acquire again. Should get new address (not addr2, unless allocator
  // reused it immediately, which is possible but unlikely to be same object
  // logic) Actually system allocator might reuse addr2, so strict equality
  // check for != addr2 might be flaky. But we can check stats if available.

  MemoryPool::PoolStats stats = pool_->get_stats();
  EXPECT_GT(stats.total_allocations, 0);
}

TEST_F(MemoryPoolLimitsTest, ValidatesSize) {
  EXPECT_THROW(pool_->acquire(0), std::invalid_argument);
  EXPECT_THROW(pool_->acquire(100 * 1024 * 1024),
               std::invalid_argument);  // > 64MB
}

}  // namespace
