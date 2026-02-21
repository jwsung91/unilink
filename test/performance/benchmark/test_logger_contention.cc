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
#include <thread>
#include <vector>

#include "unilink/diagnostics/logger.hpp"

using namespace unilink::diagnostics;

TEST(LoggerContentionTest, ConcurrentFormatMessage) {
  // Setup: Disable actual output to measure formatting contention
  Logger::instance().set_level(LogLevel::INFO);
  Logger::instance().set_outputs(0);

  const int num_threads = 4;
  const int logs_per_thread = 100000;

  auto start = std::chrono::high_resolution_clock::now();

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < logs_per_thread; ++j) {
        Logger::instance().info("TestComponent", "TestOp", "This is a test message to measure contention");
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "Concurrent logging (" << num_threads << " threads, " << logs_per_thread << " logs/thread): " << duration
            << " ms" << std::endl;

  // Restore defaults
  Logger::instance().set_outputs(static_cast<int>(LogOutput::CONSOLE));
}
