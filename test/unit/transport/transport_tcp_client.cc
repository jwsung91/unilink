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
#include <cstdint>
#include <thread>
#include <vector>

#include "test/utils/test_utils.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;
using namespace std::chrono_literals;
namespace net = boost::asio;

class TransportTcpClientTest : public ::testing::Test {
 protected:
  void TearDown() override {
    if (client_) {
      client_->stop();
      client_.reset();
    }
    TestUtils::waitFor(50);
  }

  std::shared_ptr<TcpClient> client_;
};

TEST_F(TransportTcpClientTest, BackpressureTriggersWithoutConnection) {
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 0;                       // invalid/closed port, no real connection expected
  cfg.backpressure_threshold = 1024;  // 1KB threshold

  client_ = std::make_shared<TcpClient>(cfg);
  std::atomic<bool> triggered{false};
  std::atomic<size_t> bytes_seen{0};
  client_->on_backpressure([&](size_t bytes) {
    triggered = true;
    bytes_seen = bytes;
  });

  client_->start();

  // Queue data larger than threshold to trigger backpressure on the queue
  std::vector<uint8_t> payload(cfg.backpressure_threshold * 4, 0xAA);
  client_->async_write_copy(payload.data(), payload.size());

  bool observed = TestUtils::waitForCondition(
      [&] { return triggered.load() && bytes_seen.load() >= cfg.backpressure_threshold; }, 500);

  EXPECT_TRUE(observed);

  client_->stop();
  client_.reset();
}

TEST_F(TransportTcpClientTest, StopPreventsReconnectAfterManualStop) {
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "256.256.256.256";  // force resolve failure quickly
  cfg.port = 12345;
  cfg.retry_interval_ms = 30;

  client_ = std::make_shared<TcpClient>(cfg, ioc);

  std::atomic<bool> stop_called{false};
  std::atomic<int> reconnect_after_stop{0};
  client_->on_state([&](common::LinkState state) {
    if (stop_called.load() && state == common::LinkState::Connecting) {
      reconnect_after_stop.fetch_add(1);
    }
  });

  client_->start();
  ioc.run_for(std::chrono::milliseconds(20));

  stop_called.store(true);
  client_->stop();

  // Run longer than retry interval; should not see Connecting after stop
  ioc.run_for(std::chrono::milliseconds(100));
  EXPECT_EQ(reconnect_after_stop.load(), 0);

  // Ensure client is destroyed before io_context goes out of scope
  client_.reset();
}

TEST_F(TransportTcpClientTest, ExternalIoContextFlowsThroughLifecycle) {
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "localhost";
  cfg.port = 0;  // invalid port to avoid real connect
  cfg.retry_interval_ms = 20;

  client_ = std::make_shared<TcpClient>(cfg, ioc);

  ASSERT_NO_THROW({
    client_->start();
    ioc.run_for(std::chrono::milliseconds(10));
    client_->stop();
    ioc.run_for(std::chrono::milliseconds(10));
  });

  // Destroy client before io_context is torn down to avoid dangling pointer
  client_.reset();
}

TEST_F(TransportTcpClientTest, StartStopIdempotent) {
  // Use external io_context to avoid internal thread/join issues
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "localhost";
  cfg.port = 0;  // invalid/closed port

  client_ = std::make_shared<TcpClient>(cfg, ioc);

  // Multiple start/stop cycles should be safe even without running io_context
  EXPECT_NO_THROW({
    client_->start();
    client_->start();
    client_->stop();
    client_->stop();
    client_->start();
    client_->stop();
  });

  // Destroy client before io_context is torn down to avoid dangling pointer
  client_.reset();
}

TEST_F(TransportTcpClientTest, QueueLimitMovesClientToError) {
  net::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 1;                       // no real server needed
  cfg.backpressure_threshold = 1024;  // limit becomes 1MB

  client_ = std::make_shared<TcpClient>(cfg, ioc);

  std::atomic<bool> error_seen{false};
  client_->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Error) error_seen = true;
  });

  std::vector<uint8_t> huge(cfg.backpressure_threshold * 2048, 0xEF);  // 2MB, exceeds queue cap
  client_->async_write_copy(huge.data(), huge.size());

  ioc.run_for(std::chrono::milliseconds(20));

  EXPECT_TRUE(error_seen.load());

  client_->stop();
  client_.reset();
}

TEST_F(TransportTcpClientTest, BackpressureReliefEmitsAfterDrain) {
  net::io_context ioc;
  auto guard = net::make_work_guard(ioc);
  std::thread ioc_thread([&]() { ioc.run(); });

  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 0;  // no real socket use
  cfg.backpressure_threshold = 1024;

  client_ = std::make_shared<TcpClient>(cfg, ioc);

  std::vector<size_t> bp_events;
  client_->on_backpressure([&](size_t queued) { bp_events.push_back(queued); });

  // Queue enough bytes to trigger backpressure
  std::vector<uint8_t> payload(cfg.backpressure_threshold * 2, 0xAB);
  client_->async_write_copy(payload.data(), payload.size());

  ASSERT_TRUE(TestUtils::waitForCondition([&] { return !bp_events.empty(); }, 200));

  // Stopping clears the queue and should emit a relief notification (queue = 0)
  client_->stop();
  EXPECT_TRUE(TestUtils::waitForCondition([&] { return bp_events.size() >= 2; }, 200));

  ASSERT_GE(bp_events.size(), 2);
  EXPECT_GE(bp_events.front(), cfg.backpressure_threshold);
  EXPECT_EQ(bp_events.back(), 0u);

  client_.reset();
  guard.reset();
  ioc.stop();
  if (ioc_thread.joinable()) {
    ioc_thread.join();
  }
}

TEST_F(TransportTcpClientTest, OwnedIoContextRestartAfterStopStart) {
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 0;
  cfg.max_retries = 0;  // avoid retry storm

  client_ = std::make_shared<TcpClient>(cfg);

  std::atomic<int> connecting_count{0};
  client_->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Connecting) {
      connecting_count.fetch_add(1);
    }
  });

  client_->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&] { return connecting_count.load() >= 1; }, 200));

  client_->stop();
  TestUtils::waitFor(20);

  client_->start();
  EXPECT_TRUE(TestUtils::waitForCondition([&] { return connecting_count.load() >= 2; }, 200));

  client_->stop();
}
