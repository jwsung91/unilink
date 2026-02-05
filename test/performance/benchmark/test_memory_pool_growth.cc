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
#include <iomanip>
#include <iostream>
#include <vector>

#include "unilink/memory/memory_pool.hpp"

using namespace unilink::memory;

class MemoryPoolGrowthBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(MemoryPoolGrowthBenchmark, GrowthPerformance) {
  // Force high resizing by setting initial very low and max very high
  // 4 buckets total.
  // Initial size 4 -> 1 per bucket.
  // Max size 4000 -> 1000 per bucket.
  // This will force vector reallocations as we release buffers back to pool.
  const size_t initial_size = 4;
  const size_t max_size = 4000;
  const size_t buffer_size = 1024;  // 1KB (Small)

  // We want to test the cost of pool growth.
  // The pool grows when we release buffers back to it and it's not full yet.
  // Since we start with empty pool (allocations are fresh 'new'),
  // the 'release' phase will fill the pool up to max_size.

  const int iterations = 2000;
  const int buffers_to_alloc = 1000;  // Fill one bucket completely

  std::cout << "\n=== Memory Pool Growth Performance ===" << std::endl;
  std::cout << "Initial Pool Size: " << initial_size << std::endl;
  std::cout << "Max Pool Size: " << max_size << std::endl;
  std::cout << "Alloc/Release per iter: " << buffers_to_alloc << std::endl;
  std::cout << "Iterations: " << iterations << std::endl;

  std::vector<std::unique_ptr<uint8_t[]>> buffers;
  buffers.reserve(buffers_to_alloc);

  std::chrono::nanoseconds total_duration(0);

  for (int iter = 0; iter < iterations; ++iter) {
    // Create a fresh pool for each iteration to reset vector capacity
    MemoryPool pool(initial_size, max_size);

    // Acquire buffers (this will create new ones as pool is initially empty/small)
    for (int i = 0; i < buffers_to_alloc; ++i) {
      buffers.push_back(pool.acquire(buffer_size));
    }

    auto iter_start = std::chrono::high_resolution_clock::now();
    // Release buffers (this will push back to pool's vector, triggering reallocations)
    for (auto& buf : buffers) {
      pool.release(std::move(buf), buffer_size);
    }
    auto iter_end = std::chrono::high_resolution_clock::now();
    total_duration += std::chrono::duration_cast<std::chrono::nanoseconds>(iter_end - iter_start);

    buffers.clear();
  }

  double ms_total = total_duration.count() / 1000000.0;
  double us_per_iter = (double)total_duration.count() / 1000.0 / iterations;

  std::cout << "Total Time: " << std::fixed << std::setprecision(3) << ms_total << " ms" << std::endl;
  std::cout << "Time per iteration (full fill): " << std::fixed << std::setprecision(3) << us_per_iter << " us"
            << std::endl;
}
