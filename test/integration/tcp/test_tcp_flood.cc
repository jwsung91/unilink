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
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

class TcpFloodTest : public ::testing::Test {
 protected:
  void SetUp() override { test_port_ = TestUtils::getAvailableTestPort(); }
  uint16_t test_port_;
};

TEST_F(TcpFloodTest, FloodServer) {
  std::atomic<size_t> received_count{0};
  auto server = tcp_server(test_port_)
                    .unlimited_clients()
                    .on_data([&](const wrapper::MessageContext&) { received_count++; })
                    .build();

  ASSERT_TRUE(server->start().get());

  const int num_clients = 3;
  const int messages_per_client = 20;
  std::vector<std::thread> clients;

  for (int i = 0; i < num_clients; ++i) {
    clients.emplace_back([&]() {
      auto client = tcp_client("127.0.0.1", test_port_).auto_manage(true).build();
      TestUtils::waitForCondition([&]() { return client->is_connected(); }, 5000);
      for (int j = 0; j < messages_per_client; ++j) {
        client->send("p");
        std::this_thread::sleep_for(1ms);
      }
      // Critical: wait long enough for server to process before stopping client
      std::this_thread::sleep_for(1000ms);
      client->stop();
    });
  }

  for (auto& t : clients) t.join();

  bool success =
      TestUtils::waitForCondition([&]() { return received_count.load() >= num_clients * messages_per_client; }, 10000);

  EXPECT_TRUE(success) << "Final count: " << received_count.load();
  server->stop();
}
