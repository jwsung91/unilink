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
#include <memory>
#include <sstream>
#include <vector>

#include "test_utils.hpp"
#include "unilink/config/config_factory.hpp"
#include "unilink/config/config_manager.hpp"

using namespace unilink;
using namespace unilink::config;
using namespace unilink::test;

class ConfigBenchmarkTest : public BaseTest {
 protected:
  // Helper function to format numbers with commas
  std::string formatNumber(size_t number) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << number;
    return ss.str();
  }

  // Helper function to format time duration
  std::string formatDuration(std::chrono::milliseconds duration) {
    if (duration.count() < 1000) {
      return std::to_string(duration.count()) + "ms";
    } else {
      return std::to_string(duration.count() / 1000.0) + "s";
    }
  }

  // Helper function to calculate throughput
  double calculateThroughput(size_t operations, std::chrono::milliseconds duration) {
    if (duration.count() == 0) return 0.0;
    return static_cast<double>(operations) / (duration.count() / 1000.0);
  }
};

TEST_F(ConfigBenchmarkTest, ConfigPresetsPerformance) {
  std::cout << "\n=== Config Presets Performance Benchmark ===" << std::endl;

  const size_t num_operations = 1000000;  // 1 million operations
  auto config = ConfigFactory::create();

  // Warmup
  for (int i = 0; i < 100; ++i) {
    ConfigPresets::setup_all_defaults(config);
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_operations; ++i) {
    ConfigPresets::setup_all_defaults(config);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  double throughput = calculateThroughput(num_operations, duration);

  std::cout << "Operations: " << formatNumber(num_operations) << std::endl;
  std::cout << "Duration: " << formatDuration(duration) << std::endl;
  std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " ops/sec" << std::endl;

  // Performance assertions (sanity check)
  EXPECT_GT(throughput, 1000);  // Should be at least 1K ops/sec

  std::cout << "✓ Config presets performance benchmark completed" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
