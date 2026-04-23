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

#include "unilink/memory/memory_pool.hpp"

using namespace unilink::memory;

namespace unilink {
namespace test {

TEST(MemoryPoolBranchTest, LargeAllocationBypass) {
  MemoryPool pool;
  // XLARGE is 64KB. Request 128KB.
  size_t large_size = 128 * 1024;
  auto buf = pool.acquire(large_size);
  ASSERT_NE(buf, nullptr);

  // stats check - hits should be 0, total_allocations should be 1
  auto s = pool.stats();
  EXPECT_EQ(s.pool_hits, 0);
  EXPECT_EQ(s.total_allocations, 1);

  pool.release(std::move(buf), large_size);
}

TEST(MemoryPoolBranchTest, PoolCapacityLimit) {
  // Initial pool size is dummy, max_pool_size is 4096.
  // buckets_.size() is 4. Each bucket capacity = 4096 / 4 = 1024 bytes.
  // For SMALL bucket (1024 bytes), capacity = 1024 / 1024 = 1 buffer.
  MemoryPool pool(1024, 4096);

  auto buf1 = pool.acquire(MemoryPool::BufferSize::SMALL);
  auto buf2 = pool.acquire(MemoryPool::BufferSize::SMALL);

  // Release both. One should be kept, one should be discarded.
  pool.release(std::move(buf1), static_cast<size_t>(MemoryPool::BufferSize::SMALL));
  pool.release(std::move(buf2), static_cast<size_t>(MemoryPool::BufferSize::SMALL));

  // Re-acquire. Should get at least 1 hit.
  auto buf3 = pool.acquire(MemoryPool::BufferSize::SMALL);
  auto buf4 = pool.acquire(MemoryPool::BufferSize::SMALL);

  auto s = pool.stats();
  EXPECT_GE(s.pool_hits, 1);
}

TEST(MemoryPoolBranchTest, InvalidBufferSize) {
  MemoryPool pool;
  EXPECT_THROW(pool.acquire(0), std::invalid_argument);
  EXPECT_THROW(pool.acquire(100 * 1024 * 1024), std::invalid_argument);
}

TEST(MemoryPoolBranchTest, PooledBufferBounds) {
  PooledBuffer buf(10);
  EXPECT_THROW(buf.at(10), std::out_of_range);

  const PooledBuffer& const_buf = buf;
  EXPECT_THROW(const_buf.at(10), std::out_of_range);
}

TEST(MemoryPoolBranchTest, StatsAndMetrics) {
  MemoryPool pool;
  EXPECT_EQ(pool.hit_rate(), 0.0);

  pool.acquire(MemoryPool::BufferSize::SMALL);
  EXPECT_EQ(pool.hit_rate(), 0.0);  // 0 hits / 1 alloc

  auto usage = pool.memory_usage();
  EXPECT_GT(usage.first, 0);

  auto health = pool.health_metrics();
  EXPECT_EQ(health.hit_rate, 0.0);
}

}  // namespace test
}  // namespace unilink
