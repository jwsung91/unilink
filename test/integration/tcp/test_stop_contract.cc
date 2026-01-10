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
#include "unilink/builder/unified_builder.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

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
 * Specifically targets the bug where report_backpressure(0) was called during close.
 */
TEST_F(StopContractTest, NoBackpressureCallbackAfterServerStop) {
  uint16_t port = TestUtils::getAvailableTestPort();
  std::atomic<bool> backpressure_triggered{false};
  std::atomic<int> backpressure_calls{0};
  std::atomic<bool> stop_called{false};

  // 1. Create Server using Transport Layer directly to access on_backpressure
  config::TcpServerConfig cfg;
  cfg.port = port;
  cfg.max_connections = 0;  // Unlimited

  auto server = transport::TcpServer::create(cfg);

  server->on_backpressure([&](size_t queued) {
    if (stop_called.load()) {
      // This is the CRITICAL failure condition
      // If this happens, the fix failed.
      ADD_FAILURE() << "Backpressure callback received AFTER stop! Queued: " << queued;
    }
    // Only count if backpressure state changed (e.g. triggered or relieved)
    if (queued >= cfg.backpressure_threshold || queued <= (cfg.backpressure_threshold / 2)) {
      backpressure_calls++;
    }
    if (queued >= cfg.backpressure_threshold) {
      backpressure_triggered = true;
    }
  });

  server->start();
  // Wait for server to start listening
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server->get_state() == base::LinkState::Listening; }, 1000));

  // 2. Create Client (Builder is fine here)
  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", port).build();
  client->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client->is_connected(); }, 1000));

  // 3. Trigger Backpressure
  std::string data(1024 * 1024, 'X');  // 1MB chunk

  // Send enough data to trigger backpressure (default is 4MB high watermark)
  // Sending 5 chunks (5MB) should guarantee backpressure
  for (int i = 0; i < 5; ++i) {
    server->broadcast(std::string(data.begin(), data.end()));
  }

  // Wait for backpressure to be triggered
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return backpressure_triggered.load(); }, 2000));
  // Assert that backpressure callback was actually called before stopping
  EXPECT_GT(backpressure_calls.load(), 0) << "Backpressure callback was not triggered before stop.";

  // 4. Stop Server
  stop_called = true;
  server->stop();

  // Wait a bit to ensure no late callbacks
  TestUtils::waitFor(200);

  // Cleanup is handled by test fixture / shared_ptr
}

/**
 * @brief Verify that no on_bytes callbacks occur after session is stopped.
 * This tests the race condition fix in start_read.
 */
TEST_F(StopContractTest, NoDataCallbackAfterServerStop) {
  uint16_t port = TestUtils::getAvailableTestPort();
  std::atomic<bool> stop_called{false};
  std::atomic<int> data_calls{0};

  auto server = builder::UnifiedBuilder::tcp_server(port)
                    .unlimited_clients()
                    .on_data([&](const std::string& data) {
                      if (stop_called.load()) {
                        ADD_FAILURE() << "Data callback received AFTER stop! Size: " << data.size();
                      }
                      data_calls++;
                      // Simulate work to widen the race window
                      std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    })
                    .build();

  server->start();
  // Wait for server to be listening
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server->is_listening(); }, 1000));

  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", port).build();
  client->start();
  // Wait for client to connect
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client->is_connected(); }, 1000));

  // Client sends data continuously
  std::atomic<bool> sending{true};
  std::thread sender([&]() {
    std::string chunk(1024, 'A');
    while (sending.load()) {
      client->send(chunk);
      // Small delay to allow receiver to process and keep the stream flowing
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });

  // Let some data flow and verify it's being received
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return data_calls.load() > 5; }, 2000));
  EXPECT_GT(data_calls.load(), 0) << "Data callbacks were not triggered before stop.";

  // TRIGGER STOP
  stop_called = true;
  server->stop();

  // Stop sender thread
  sending = false;
  if (sender.joinable()) {
    sender.join();
  }
  // Stop client explicitly
  client->stop();

  // Wait to ensure no late callbacks
  TestUtils::waitFor(200);

  // Cleanup is handled by test fixture / shared_ptr
}
