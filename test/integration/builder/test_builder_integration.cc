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

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

class UnifiedBuilderIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    port_ = TestUtils::getAvailableTestPort();
  }
  uint16_t port_;
};

TEST_F(UnifiedBuilderIntegrationTest, RealCommunicationBetweenBuilderObjects) {
  std::atomic<bool> data_received{false};
  std::string received_msg;

  auto server = builder::UnifiedBuilder::tcp_server(port_)
                    .unlimited_clients()
                    .on_data([&](const wrapper::MessageContext& ctx) {
                      received_msg = std::string(ctx.data());
                      data_received = true;
                    })
                    .build();

  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", port_).build();

  ASSERT_TRUE(server->start().get());
  client->start();

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client->is_connected(); }, 2000));
  
  client->send("hello from unified");
  
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return data_received.load(); }, 2000));
  EXPECT_EQ(received_msg, "hello from unified");

  client->stop();
  server->stop();
}

TEST_F(UnifiedBuilderIntegrationTest, BuilderConfigurationAffectsCommunication) {
  // Test that settings like retry interval are actually applied
  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", port_)
                    .retry_interval(100)
                    .build();
  
  // Start without server, it should retry quickly
  client->start();
  TestUtils::waitFor(300);
  
  auto server = builder::UnifiedBuilder::tcp_server(port_).build();
  server->start();
  
  // Client should eventually connect via retry
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client->is_connected(); }, 5000));
  
  client->stop();
  server->stop();
}
