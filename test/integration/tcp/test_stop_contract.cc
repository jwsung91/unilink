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
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/base/common.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;

/**
 * @brief Integration tests verifying "No Callbacks after Stop" contract.
 */
class StopContractTest : public BaseTest {
 protected:
  void SetUp() override {
    BaseTest::SetUp();
    auto& logger = unilink::diagnostics::Logger::instance();
    previous_log_level_ = logger.get_level();
    // Enable debug logging for detailed trace
    logger.set_level(unilink::diagnostics::LogLevel::DEBUG);
  }

  void TearDown() override {
    unilink::diagnostics::Logger::instance().set_level(previous_log_level_);
    BaseTest::TearDown();
  }

 private:
  unilink::diagnostics::LogLevel previous_log_level_{unilink::diagnostics::LogLevel::INFO};
};

/**
 * @brief Verify that no backpressure callbacks occur after TcpServer stop.
 */
TEST_F(StopContractTest, NoBackpressureCallbackAfterServerStop) {
  uint16_t port = TestUtils::getAvailableTestPort();
  std::atomic<bool> backpressure_triggered{false};
  std::atomic<int> backpressure_calls{0};
  std::atomic<bool> stop_called{false};

  config::TcpServerConfig cfg;
  cfg.port = port;
  cfg.max_connections = 0;

  auto server = transport::TcpServer::create(cfg);

  server->on_backpressure([&](size_t queued) {
    if (stop_called.load()) {
      ADD_FAILURE() << "Backpressure callback received AFTER stop! Queued: " << queued;
    }
    if (queued >= cfg.backpressure_threshold || queued <= (cfg.backpressure_threshold / 2)) {
      backpressure_calls++;
    }
    if (queued >= cfg.backpressure_threshold) {
      backpressure_triggered = true;
    }
  });

  server->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server->get_state() == base::LinkState::Listening; }, 1000));

  auto client = tcp_client("127.0.0.1", port).build();
  client->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client->is_connected(); }, 1000));

  std::string data(1024 * 1024, 'X');
  for (int i = 0; i < 5; ++i) {
    server->broadcast(std::string(data.begin(), data.end()));
  }

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return backpressure_triggered.load(); }, 2000));
  EXPECT_GT(backpressure_calls.load(), 0);

  server->stop();
  stop_called = true;
  TestUtils::waitFor(200);
}

/**
 * @brief Verify that no on_bytes callbacks occur after session is stopped.
 */
TEST_F(StopContractTest, NoDataCallbackAfterServerStop) {
  uint16_t port = TestUtils::getAvailableTestPort();
  std::atomic<bool> stop_called{false};
  std::atomic<int> data_calls{0};

  auto server = tcp_server(port)
                    .unlimited_clients()
                    .on_data([&](const wrapper::MessageContext& ctx) {
                      if (stop_called.load()) {
                        ADD_FAILURE() << "Data callback received AFTER stop! Size: " << ctx.data().size();
                      }
                      data_calls++;
                      std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    })
                    .build();

  server->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server->is_listening(); }, 1000));

  auto client = tcp_client("127.0.0.1", port).build();
  client->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client->is_connected(); }, 1000));

  std::atomic<bool> sending{true};
  std::thread sender([&]() {
    std::string chunk(1024, 'A');
    while (sending.load()) {
      client->send(chunk);
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return data_calls.load() > 5; }, 2000));

  server->stop();
  stop_called = true;

  sending = false;
  if (sender.joinable()) sender.join();
  client->stop();
  TestUtils::waitFor(200);
}
