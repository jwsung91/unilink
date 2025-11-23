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
#include <vector>

#include "test/utils/test_utils.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;
using namespace std::chrono_literals;

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
}
