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

class LoggerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset logger state
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_outputs(0);  // Disable outputs to measure only overhead
  }

  void TearDown() override {
    // Restore default state
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_outputs(static_cast<int>(LogOutput::CONSOLE));
  }
};

TEST_F(LoggerPerfTest, PerfMacroOverheadDisabled) {
  // Ensure logging is disabled for DEBUG level
  Logger::instance().set_level(LogLevel::INFO);

  auto start = std::chrono::high_resolution_clock::now();

  std::string opName = "DisabledOp";

  const int iterations = 1000000;
  for (int i = 0; i < iterations; ++i) {
    UNILINK_LOG_PERF_START("PerfTest", opName);
    // Simulate some very minimal work to ensure start/end are distinct enough
    // but keep it minimal to focus on macro overhead
    UNILINK_LOG_PERF_END("PerfTest", opName);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  std::cout << "Disabled Logging (1M iter): " << duration << " μs (" << (double)duration / iterations << " μs/call)"
            << std::endl;
}

TEST_F(LoggerPerfTest, PerfMacroOverheadEnabled) {
  // Enable logging for DEBUG level
  Logger::instance().set_level(LogLevel::DEBUG);

  auto start = std::chrono::high_resolution_clock::now();

  std::string opName = "EnabledOp";

  const int iterations = 100000;
  for (int i = 0; i < iterations; ++i) {
    UNILINK_LOG_PERF_START("PerfTest", opName);
    UNILINK_LOG_PERF_END("PerfTest", opName);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  std::cout << "Enabled Logging (100k iter): " << duration << " μs (" << (double)duration / iterations << " μs/call)"
            << std::endl;
}
