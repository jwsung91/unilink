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

#include "unilink/util/input_validator.hpp"

using namespace unilink::util;

class IPv6ValidationBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {
    // Warm up
    try {
      InputValidator::validate_ipv6_address("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    } catch (...) {}
  }
};

TEST_F(IPv6ValidationBenchmark, Performance) {
  // Use a reasonable iteration count for benchmark
  const int iterations = 10000;

  // Use mostly valid addresses to avoid exception overhead masking regex performance
  std::vector<std::string> valid_addresses = {
    "2001:0db8:85a3:0000:0000:8a2e:0370:7334",
    "fe80:0000:0000:0000:0202:b3ff:fe1e:8329",
    "0000:0000:0000:0000:0000:0000:0000:0001",
    "::1",
    "::"
  };

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    for (const auto& addr : valid_addresses) {
      try {
        InputValidator::validate_ipv6_address(addr);
      } catch (const std::exception& e) {
        // Should not happen for valid addresses
        FAIL() << "Unexpected exception for address " << addr << ": " << e.what();
      }
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "\n=== IPv6 Validation Performance ===" << std::endl;
  std::cout << "Iterations: " << iterations << std::endl;
  std::cout << "Addresses per iteration: " << valid_addresses.size() << std::endl;
  std::cout << "Total validations: " << iterations * valid_addresses.size() << std::endl;
  std::cout << "Total time: " << duration << " ms" << std::endl;

  double total_ops = static_cast<double>(iterations) * valid_addresses.size();
  if (total_ops > 0) {
    double ns_per_op = (static_cast<double>(duration) * 1000000.0) / total_ops;
    std::cout << "Average time per validation: " << ns_per_op << " ns" << std::endl;
  }
  std::cout << "===================================\n" << std::endl;
}
