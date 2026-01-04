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
#include <exception>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/builder/unified_builder.hpp"
#include "unilink/base/constants.hpp"
#include "unilink/diagnostics/exceptions.hpp"
#include "unilink/util/input_validator.hpp"
#include "unilink/config/serial_config.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::transport;
using namespace unilink::config;
using namespace unilink::diagnostics;
using namespace std::chrono_literals;

/**
 * @brief Error recovery and resilience tests
 */
class ErrorRecoveryTest : public BaseTest {
 protected:
  void SetUp() override {
    BaseTest::SetUp();
    error_count_ = 0;
  }
  std::atomic<int> error_count_{0};
};

TEST_F(ErrorRecoveryTest, NetworkConnectionErrors) {
  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", 1)
                     .on_error([this](const std::string& error) {
                       error_count_++;
                     })
                     .build();
  ASSERT_NE(client, nullptr);
  client->start();
  TestUtils::waitFor(500);
}

TEST_F(ErrorRecoveryTest, MemoryAllocationFailureHandling) {
  auto& pool = memory::GlobalMemoryPool::instance();
  auto buffer = pool.acquire(1024);
  EXPECT_NE(buffer, nullptr);
  if (buffer) {
    pool.release(std::move(buffer), 1024);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}