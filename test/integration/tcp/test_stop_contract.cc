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
class StopContractTest : public BaseTest {};

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
  // Use a low backpressure threshold to trigger it easily (if configurable)
  // Default is 4MB. We can't change it easily via config struct unless it has a field.
  // Assuming default.

  auto server = transport::TcpServer::create(cfg);

  server->on_backpressure([&](size_t queued) {
    if (stop_called.load()) {
      // This is the CRITICAL failure condition
      // If this happens, the fix failed.
      ADD_FAILURE() << "Backpressure callback received AFTER stop! Queued: " << queued;
    }
    backpressure_triggered = true;
    backpressure_calls++;
  });

  server->start();
  TestUtils::waitFor(100);

  // 2. Create Client (Builder is fine here)
  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", port).build();
  client->start();
  TestUtils::waitFor(100);

  // 3. Trigger Backpressure
  // Generate large data
  std::vector<uint8_t> data(1024 * 1024, 'X');  // 1MB

  // Fill the queue
  // We broadcast to all (1) clients.
  // Transport layer broadcast returns bool.
  for (int i = 0; i < 10; ++i) {
    server->broadcast(std::string(data.begin(), data.end()));
  }

  // Wait a bit for backpressure to likely trigger
  TestUtils::waitFor(100);

  // 4. Stop Server
  stop_called = true;
  server->stop();

  // Wait a bit to see if callback fires
  TestUtils::waitFor(200);

  // Clean up
  client->stop();
  // server is already stopped
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

  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", port).build();
  client->start();
  TestUtils::waitFor(100);

  // Client sends data continuously
  std::atomic<bool> sending{true};
  std::thread sender([&]() {
    std::string chunk(1024, 'A');
    while (sending) {
      client->send(chunk);
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });

  // Let some data flow
  TestUtils::waitFor(500);

  // TRIGGER STOP
  stop_called = true;
  server->stop();

  // Stop sender
  sending = false;
  sender.join();

  // Wait to ensure no late callbacks
  TestUtils::waitFor(200);
}
