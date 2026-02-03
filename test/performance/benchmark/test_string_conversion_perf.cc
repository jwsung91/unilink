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
#include <string>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/memory/safe_span.hpp"

using namespace unilink;
using namespace std::chrono;

class StringConversionPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a reasonably large string to make allocation cost visible
    test_string_ = std::string(1024 * 10, 'A');  // 10KB
  }

  std::string test_string_;
  const int iterations_ = 100000;
};

TEST_F(StringConversionPerfTest, OriginalVectorAllocation) {
  auto start = high_resolution_clock::now();

  volatile size_t check_sum = 0;
  for (int i = 0; i < iterations_; ++i) {
    auto vec = base::safe_convert::string_to_uint8(test_string_);
    check_sum += vec.size();
    // Force usage
    if (!vec.empty()) check_sum += vec[0];
  }

  auto end = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(end - start).count();

  std::cout << "[ PERF     ] Original (Vector): " << duration << " us for " << iterations_ << " iterations"
            << std::endl;
}

TEST_F(StringConversionPerfTest, OptimizedSpan) {
  auto start = high_resolution_clock::now();

  volatile size_t check_sum = 0;
  for (int i = 0; i < iterations_; ++i) {
    auto span = base::safe_convert::string_to_uint8_span(test_string_);
    check_sum += span.size();
    // Force usage
    if (!span.empty()) check_sum += span[0];
  }

  auto end = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(end - start).count();

  std::cout << "[ PERF     ] Optimized (Span): " << duration << " us for " << iterations_ << " iterations" << std::endl;
}
