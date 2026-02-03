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
#include "unilink/diagnostics/logger.hpp"

using namespace unilink::diagnostics;

class ConsoleLoggerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Ensure logger is in a clean state
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_outputs(static_cast<int>(LogOutput::CONSOLE));
  }

  void TearDown() override {
    // Restore default state
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_outputs(static_cast<int>(LogOutput::CONSOLE));
  }
};

TEST_F(ConsoleLoggerPerfTest, ConsoleOutputPerformance) {
  // We log a sufficient number of messages to make the I/O cost dominant and measurable.
  const int iterations = 5000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
     Logger::instance().info("TestComponent", "TestOperation", "This is a test message to measure console output performance.");
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "Console Logging (" << iterations << " iter): " << duration << " ms ("
            << (double)duration / iterations * 1000.0 << " μs/call)" << std::endl;
}
