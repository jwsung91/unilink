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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/diagnostics/exceptions.hpp"
#include "unilink/memory/safe_data_buffer.hpp"
#include "unilink/unilink.hpp"
#include "unilink/util/input_validator.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::memory;
using namespace unilink::diagnostics;
using namespace unilink::builder;
using namespace std::chrono_literals;

/**
 * @brief Integrated safety-related tests
 */
class SafetyIntegratedTest : public ::testing::Test {
 protected:
  void SetUp() override { test_port_ = TestUtils::getAvailableTestPort(); }

  void TearDown() override { TestUtils::waitFor(1000); }

  uint16_t test_port_;
};

TEST_F(SafetyIntegratedTest, ApiSafetyNullPointers) {
  auto client = unilink::tcp_client("127.0.0.1", test_port_).build();
  EXPECT_NE(client, nullptr);

  auto server = unilink::tcp_server(test_port_).unlimited_clients().build();
  EXPECT_NE(server, nullptr);
}

TEST_F(SafetyIntegratedTest, ApiSafetyInvalidParameters) {
  EXPECT_THROW(auto client = unilink::tcp_client("127.0.0.1", 0).build(), BuilderException);
}

TEST_F(SafetyIntegratedTest, ConcurrencySafetyClientCreation) {
  const int num_threads = 4;
  const int clients_per_thread = 10;
  std::atomic<int> success_count{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < clients_per_thread; ++i) {
        auto client = unilink::tcp_client("127.0.0.1", test_port_ + i).build();
        if (client) success_count++;
      }
    });
  }

  for (auto& thread : threads) thread.join();
  EXPECT_EQ(success_count.load(), num_threads * clients_per_thread);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}