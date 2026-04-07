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
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "test_utils.hpp"
#include "unilink/builder/auto_initializer.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

class UdsIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    socket_path_ =
        "/tmp/unilink_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".sock";
    std::remove(socket_path_.c_str());
  }

  void TearDown() override { std::remove(socket_path_.c_str()); }

  std::string socket_path_;
};

TEST_F(UdsIntegrationTest, BuilderPatternIntegration) {
  auto server = unilink::uds_server(socket_path_).unlimited_clients().build();
  EXPECT_NE(server, nullptr);

  auto client = unilink::uds_client(socket_path_).build();
  EXPECT_NE(client, nullptr);
}

TEST_F(UdsIntegrationTest, BasicCommunication) {
  std::atomic<bool> server_connected{false};
  std::atomic<bool> client_connected{false};
  std::atomic<bool> data_received{false};
  std::string received_data;
  std::mutex mtx;
  std::condition_variable cv;

  auto server = unilink::uds_server(socket_path_)
                    .on_connect([&server_connected](const wrapper::ConnectionContext&) { server_connected = true; })
                    .on_data([&](const wrapper::MessageContext& ctx) {
                      std::lock_guard<std::mutex> lock(mtx);
                      received_data = std::string(ctx.data());
                      data_received = true;
                      cv.notify_one();
                    })
                    .build();

  server->start();

  // Wait for server to be ready
  bool listening = TestUtils::waitForCondition([&]() { return server->is_listening(); }, 2000);
  ASSERT_TRUE(listening) << "Server failed to start listening";

  auto client = unilink::uds_client(socket_path_)
                    .use_independent_context(true)
                    .on_connect([&client_connected](const wrapper::ConnectionContext&) { client_connected = true; })
                    .build();

  client->start();

  // Wait for connection
  bool connected = TestUtils::waitForCondition([&]() { return server_connected && client_connected; }, 5000);
  ASSERT_TRUE(connected) << "Failed to establish connection";

  // Send data
  std::string test_msg = "Hello UDS!";
  client->send(test_msg);

  // Wait for data
  std::unique_lock<std::mutex> lock(mtx);
  bool success = cv.wait_for(lock, 5s, [&]() { return data_received.load(); });

  EXPECT_TRUE(success) << "Data was not received by server";
  EXPECT_EQ(received_data, test_msg);

  client->stop();
  server->stop();
}

TEST_F(UdsIntegrationTest, MultiClientCommunication) {
  std::atomic<int> connections{0};
  std::atomic<int> messages_received{0};
  std::mutex mtx;
  std::condition_variable cv;

  auto server = unilink::uds_server(socket_path_)
                    .unlimited_clients()
                    .on_connect([&connections](const wrapper::ConnectionContext&) { connections++; })
                    .on_data([&](const wrapper::MessageContext& ctx) { messages_received++; })
                    .build();

  server->start();
  TestUtils::waitForCondition([&]() { return server->is_listening(); }, 2000);

  auto client1 = unilink::uds_client(socket_path_).use_independent_context(true).build();
  auto client2 = unilink::uds_client(socket_path_).use_independent_context(true).build();

  client1->start();
  client2->start();

  bool connected = TestUtils::waitForCondition([&]() { return connections == 2; }, 5000);
  EXPECT_TRUE(connected);
  EXPECT_EQ(connections, 2);

  client1->send("Msg1");
  client2->send("Msg2");

  bool received = TestUtils::waitForCondition([&]() { return messages_received == 2; }, 5000);
  EXPECT_TRUE(received);
  EXPECT_EQ(messages_received, 2);

  client1->stop();
  client2->stop();
  server->stop();
}
