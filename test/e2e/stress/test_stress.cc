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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/base/common.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

class StressTest : public BaseTest {
 protected:
  void SetUp() override {
    BaseTest::SetUp();
    auto& pool = memory::GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));
  }

  void TearDown() override {
    auto& pool = memory::GlobalMemoryPool::instance();
    pool.cleanup_old_buffers(std::chrono::milliseconds(0));
    BaseTest::TearDown();
  }
};

TEST_F(StressTest, RealNetworkHighThroughput) {
  const uint16_t port = TestUtils::getAvailableTestPort();
  const size_t chunk_size = 64 * 1024;
  const int chunk_count = 50;

  std::atomic<size_t> server_received_bytes{0};

  auto server = tcp_server(port)
                    .unlimited_clients()
                    .on_data([&](const wrapper::MessageContext& ctx) { server_received_bytes += ctx.data().size(); })
                    .build();

  ASSERT_NE(server, nullptr);
  server->start();
  TestUtils::waitFor(100);

  std::atomic<bool> client_connected{false};
  auto client =
      tcp_client("127.0.0.1", port)
          .on_connect([&](const wrapper::ConnectionContext&) { client_connected = true; })
          .build();

  ASSERT_NE(client, nullptr);
  client->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client_connected.load(); }, 5000));

  std::string chunk(chunk_size, 'X');
  auto target_bytes = chunk_size * chunk_count;

  for (int i = 0; i < chunk_count; ++i) {
    client->send(chunk);
    std::this_thread::sleep_for(std::chrono::microseconds(500));
  }

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server_received_bytes.load() >= target_bytes; }, 10000));
  EXPECT_EQ(server_received_bytes.load(), target_bytes);

  client->stop();
  server->stop();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
