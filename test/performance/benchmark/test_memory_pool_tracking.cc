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

#include <chrono>
#include <iostream>
#include <vector>

#include "unilink/memory/memory_pool.hpp"

using namespace unilink::memory;

class MemoryPoolTrackingBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(MemoryPoolTrackingBenchmark, AllocationOverhead) {
  // We want to measure the overhead of memory tracking during allocation.
  // We'll create a fresh pool and acquire buffers, which forces 'create_buffer'
  // and thus triggers MEMORY_TRACK_ALLOCATION.

  const size_t buffer_size = 1024;  // 1KB
  const int iterations = 1000;      // Number of pool creations
  const int allocs_per_iter = 100;  // Allocations per pool

  std::vector<std::unique_ptr<uint8_t[]>> buffers;
  buffers.reserve(allocs_per_iter);

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    MemoryPool pool(4, 10000);  // Use local pool to start fresh each time
    for (int j = 0; j < allocs_per_iter; ++j) {
      buffers.push_back(pool.acquire(buffer_size));
    }
    // Clean up buffers (they are unique_ptrs, so this releases them)
    // Note: buffers are destroyed here. Since they were allocated from 'pool',
    // and 'pool' is still alive (in scope), they are returned to 'pool'.
    // Then 'pool' is destroyed at end of loop.
    buffers.clear();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  size_t total_ops = iterations * allocs_per_iter;
  std::cout << "Allocation Time (" << total_ops << " ops): " << duration << " ms" << std::endl;
  std::cout << "Time per op: " << (double)duration * 1000000 / total_ops << " ns" << std::endl;
}
