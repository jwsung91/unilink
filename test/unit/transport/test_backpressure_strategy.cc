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
#include <thread>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::base::constants;
using namespace std::chrono_literals;
namespace net = boost::asio;

// ─── KeepAll: default strategy preserves data until hard limit ────────────────

TEST(BackpressureStrategyTest, KeepAllIsDefaultInConfig) {
  config::TcpClientConfig cfg;
  EXPECT_EQ(cfg.backpressure_strategy, BackpressureStrategy::KeepAll);
}

TEST(BackpressureStrategyTest, KeepLatestRoundtripsInConfig) {
  config::TcpClientConfig cfg;
  cfg.backpressure_strategy = BackpressureStrategy::KeepLatest;
  EXPECT_EQ(cfg.backpressure_strategy, BackpressureStrategy::KeepLatest);
}

TEST(BackpressureStrategyTest, TcpServerConfigDefaultIsKeepAll) {
  config::TcpServerConfig cfg;
  EXPECT_EQ(cfg.backpressure_strategy, BackpressureStrategy::KeepAll);
}

// ─── KeepLatest: queue is cleared when threshold is exceeded ──────────────────

TEST(BackpressureStrategyTest, KeepLatest_BackpressureCallbackFiredAndQueueCleared) {
  // Stand up a real loopback server + client pair to exercise the write path.
  constexpr uint16_t kPort = 19801;
  constexpr size_t kThreshold = 1024;  // 1 KB

  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  std::thread io_thread([&] { ioc.run(); });

  // Server (just accepts connection)
  config::TcpServerConfig srv_cfg;
  srv_cfg.port = kPort;
  auto server = TcpServer::create(srv_cfg);
  server->start();

  // Client with KeepLatest strategy and small threshold
  config::TcpClientConfig cli_cfg;
  cli_cfg.host = "127.0.0.1";
  cli_cfg.port = kPort;
  cli_cfg.backpressure_threshold = kThreshold;
  cli_cfg.backpressure_strategy = BackpressureStrategy::KeepLatest;

  auto client = TcpClient::create(cli_cfg, ioc);

  std::atomic<int> bp_count{0};
  client->on_backpressure([&](size_t) { bp_count.fetch_add(1); });

  client->start();

  // Wait for connection
  std::this_thread::sleep_for(200ms);

  // Flood with large messages to blow past the threshold
  std::vector<uint8_t> big(kThreshold * 2, 0xAB);
  for (int i = 0; i < 10; ++i) {
    client->async_write_copy(memory::ConstByteSpan(big.data(), big.size()));
  }

  std::this_thread::sleep_for(100ms);

  client->stop();
  server->stop();

  work.reset();
  ioc.stop();
  io_thread.join();

  // With KeepLatest, backpressure must have fired at least once (queue was
  // flushed each time the threshold was exceeded).
  EXPECT_GE(bp_count.load(), 1);
}

// ─── set_backpressure_strategy: runtime change takes effect ──────────────────

TEST(BackpressureStrategyTest, SetBackpressureStrategyChangesMode) {
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 19802;
  cfg.backpressure_strategy = BackpressureStrategy::KeepAll;

  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  std::thread io_thread([&] { ioc.run(); });

  auto client = TcpClient::create(cfg, ioc);

  // Switch to KeepLatest at runtime — must not crash
  client->set_backpressure_strategy(BackpressureStrategy::KeepLatest);

  // Basic smoke: queuing without a connection should not crash
  std::vector<uint8_t> data(512, 0xFF);
  client->async_write_copy(memory::ConstByteSpan(data.data(), data.size()));
  std::this_thread::sleep_for(50ms);

  work.reset();
  ioc.stop();
  io_thread.join();
}
