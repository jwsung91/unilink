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
#include <string>
#include <vector>

#include "unilink/memory/memory_tracker.hpp"

using namespace unilink::memory;

class MemoryTrackerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Ensure tracking is enabled
    MemoryTracker::instance().enable_tracking(true);
    // Clear any previous data
    MemoryTracker::instance().clear_tracking_data();
  }

  void TearDown() override { MemoryTracker::instance().clear_tracking_data(); }
};

TEST_F(MemoryTrackerPerfTest, TrackAllocationOverhead) {
  const size_t iterations = 1000000;
  std::vector<void*> ptrs;
  ptrs.reserve(iterations);

  // Pre-allocate pointers to avoid measuring vector allocation time
  for (size_t i = 0; i < iterations; ++i) {
    ptrs.push_back(reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1)));
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < iterations; ++i) {
    MemoryTracker::instance().track_allocation(ptrs[i], 64, __FILE__, __LINE__, __FUNCTION__);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "Track Allocation Time (" << iterations << " ops): " << duration << " ms" << std::endl;
  std::cout << "Time per op: " << (double)duration * 1000000 / iterations << " ns" << std::endl;

  // Cleanup to avoid memory usage growth during test
  for (size_t i = 0; i < iterations; ++i) {
    MemoryTracker::instance().track_deallocation(ptrs[i]);
  }
}

TEST_F(MemoryTrackerPerfTest, TrackDeallocationOverhead) {
  const size_t iterations = 1000000;
  std::vector<void*> ptrs;
  ptrs.reserve(iterations);

  for (size_t i = 0; i < iterations; ++i) {
    void* ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1));
    ptrs.push_back(ptr);
    MemoryTracker::instance().track_allocation(ptr, 64, __FILE__, __LINE__, __FUNCTION__);
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < iterations; ++i) {
    MemoryTracker::instance().track_deallocation(ptrs[i]);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "Track Deallocation Time (" << iterations << " ops): " << duration << " ms" << std::endl;
  std::cout << "Time per op: " << (double)duration * 1000000 / iterations << " ns" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
