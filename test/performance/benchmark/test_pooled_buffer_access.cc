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
#include <vector>
#include <iostream>
#include <iomanip>
#include <numeric>

#include "unilink/memory/memory_pool.hpp"

using namespace unilink::memory;

class PooledBufferAccessBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {
    // Warm up pool
    GlobalMemoryPool::instance().acquire(1024);
  }
};

TEST_F(PooledBufferAccessBenchmark, AccessPerformance) {
  const size_t buffer_size = 4096; // 4KB
  const size_t iterations = 100000;

  auto& pool = GlobalMemoryPool::instance();
  // Ensure we get a buffer
  auto pooled_buf = PooledBuffer(buffer_size);
  ASSERT_TRUE(pooled_buf.valid());

  // Fill with some data
  for(size_t i = 0; i < buffer_size; ++i) {
    pooled_buf[i] = static_cast<uint8_t>(i % 256);
  }

  std::cout << "\n=== PooledBuffer Access Performance ===" << std::endl;
  std::cout << "Buffer Size: " << buffer_size << " bytes" << std::endl;
  std::cout << "Iterations: " << iterations << std::endl;

  // Benchmark operator[]
  auto start_bracket = std::chrono::high_resolution_clock::now();

  volatile uint8_t sum_bracket = 0;
  for(size_t iter = 0; iter < iterations; ++iter) {
    for(size_t i = 0; i < buffer_size; ++i) {
      sum_bracket += pooled_buf[i];
    }
  }

  auto end_bracket = std::chrono::high_resolution_clock::now();
  auto duration_bracket = std::chrono::duration_cast<std::chrono::microseconds>(end_bracket - start_bracket);

  // Benchmark at()
  auto start_at = std::chrono::high_resolution_clock::now();

  volatile uint8_t sum_at = 0;
  for(size_t iter = 0; iter < iterations; ++iter) {
    for(size_t i = 0; i < buffer_size; ++i) {
      sum_at += pooled_buf.at(i);
    }
  }

  auto end_at = std::chrono::high_resolution_clock::now();
  auto duration_at = std::chrono::duration_cast<std::chrono::microseconds>(end_at - start_at);

  double ns_per_op_bracket = (double)duration_bracket.count() * 1000.0 / (iterations * buffer_size);
  double ns_per_op_at = (double)duration_at.count() * 1000.0 / (iterations * buffer_size);

  std::cout << "operator[] Total Time: " << duration_bracket.count() / 1000.0 << " ms" << std::endl;
  std::cout << "operator[] Time per op: " << std::fixed << std::setprecision(3) << ns_per_op_bracket << " ns" << std::endl;

  std::cout << "at() Total Time: " << duration_at.count() / 1000.0 << " ms" << std::endl;
  std::cout << "at() Time per op: " << std::fixed << std::setprecision(3) << ns_per_op_at << " ns" << std::endl;

  double improvement = (ns_per_op_at - ns_per_op_bracket) / ns_per_op_at * 100.0;

  std::cout << "Relative Difference (at vs []): " << improvement << "%" << std::endl;
}
