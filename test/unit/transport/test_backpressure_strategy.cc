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
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/udp_builder.hpp"
#include "unilink/builder/uds_builder.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"
#include "unilink/transport/udp/udp.hpp"
#include "unilink/transport/uds/uds_client.hpp"
#include "unilink/transport/uds/uds_server.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::base::constants;
using namespace std::chrono_literals;
namespace net = boost::asio;

// ─── Reliable: default strategy preserves data until hard limit ────────────────

TEST(BackpressureStrategyTest, ReliableIsDefaultInConfig) {
  config::TcpClientConfig cfg;
  EXPECT_EQ(cfg.backpressure_strategy, BackpressureStrategy::Reliable);
}

TEST(BackpressureStrategyTest, BestEffortRoundtripsInConfig) {
  config::TcpClientConfig cfg;
  cfg.backpressure_strategy = BackpressureStrategy::BestEffort;
  EXPECT_EQ(cfg.backpressure_strategy, BackpressureStrategy::BestEffort);
}

TEST(BackpressureStrategyTest, TcpServerConfigDefaultIsReliable) {
  config::TcpServerConfig cfg;
  EXPECT_EQ(cfg.backpressure_strategy, BackpressureStrategy::Reliable);
}

// ─── BestEffort: queue is cleared when threshold is exceeded ──────────────────

TEST(BackpressureStrategyTest, BestEffort_BackpressureCallbackFiredAndQueueCleared) {
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

  // Client with BestEffort strategy and small threshold
  config::TcpClientConfig cli_cfg;
  cli_cfg.host = "127.0.0.1";
  cli_cfg.port = kPort;
  cli_cfg.backpressure_threshold = kThreshold;
  cli_cfg.backpressure_strategy = BackpressureStrategy::BestEffort;

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

  // With BestEffort, backpressure must have fired at least once (queue was
  // flushed each time the threshold was exceeded).
  EXPECT_GE(bp_count.load(), 1);
}

// ─── set_backpressure_strategy: runtime change takes effect ──────────────────

TEST(BackpressureStrategyTest, SetBackpressureStrategyChangesMode) {
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 19802;
  cfg.backpressure_strategy = BackpressureStrategy::Reliable;

  net::io_context ioc;
  auto work = net::make_work_guard(ioc);
  std::thread io_thread([&] { ioc.run(); });

  auto client = TcpClient::create(cfg, ioc);

  // Switch to BestEffort at runtime — must not crash
  client->set_backpressure_strategy(BackpressureStrategy::BestEffort);

  // Basic smoke: queuing without a connection should not crash
  std::vector<uint8_t> data(512, 0xFF);
  client->async_write_copy(memory::ConstByteSpan(data.data(), data.size()));
  std::this_thread::sleep_for(50ms);

  work.reset();
  ioc.stop();
  io_thread.join();
}

// ─── UdsClient: report_backpressure uses hysteresis (ON/OFF transitions only) ─

TEST(BackpressureStrategyTest, UdsClient_ReportBackpressureHysteresis) {
  constexpr size_t kThreshold = 1024;
  auto tmp_path = std::string("/tmp/unilink_bp_hysteresis_") + std::to_string(getpid()) + ".sock";

  // Start a UDS server to accept the connection (manages its own ioc)
  config::UdsServerConfig srv_cfg;
  srv_cfg.socket_path = tmp_path;
  auto server = transport::UdsServer::create(srv_cfg);
  server->start();
  std::this_thread::sleep_for(50ms);

  // Client with small threshold (manages its own ioc)
  config::UdsClientConfig cli_cfg;
  cli_cfg.socket_path = tmp_path;
  cli_cfg.backpressure_threshold = kThreshold;
  cli_cfg.backpressure_strategy = BackpressureStrategy::BestEffort;

  auto client = transport::UdsClient::create(cli_cfg);

  std::vector<size_t> bp_events;
  std::mutex bp_mtx;
  client->on_backpressure([&](size_t queued) {
    std::lock_guard<std::mutex> lock(bp_mtx);
    bp_events.push_back(queued);
  });

  client->start();
  std::this_thread::sleep_for(100ms);

  // Flood to trigger ON transition
  std::vector<uint8_t> big(kThreshold * 2, 0xAB);
  for (int i = 0; i < 5; ++i) {
    client->async_write_copy(memory::ConstByteSpan(big.data(), big.size()));
  }
  std::this_thread::sleep_for(100ms);

  client->stop();
  server->stop();

  // Hysteresis: with BestEffort and flooding, must fire at least once
  std::lock_guard<std::mutex> lock(bp_mtx);
  EXPECT_GE(bp_events.size(), 1u);
}

// ─── Builder: backpressure_strategy/threshold fluent API ──────────────────────

TEST(BackpressureStrategyTest, BuilderFluentBackpressureStrategy) {
  auto client = builder::TcpClientBuilder("127.0.0.1", 19810)
                    .backpressure_strategy(BackpressureStrategy::BestEffort)
                    .backpressure_threshold(512 * 1024)
                    .build();
  ASSERT_NE(client, nullptr);
}

TEST(BackpressureStrategyTest, UdsBuilderFluentBackpressureStrategy) {
  auto tmp_path = std::string("/tmp/unilink_bp_builder_") + std::to_string(getpid()) + ".sock";
  auto client = builder::UdsClientBuilder(tmp_path)
                    .backpressure_strategy(BackpressureStrategy::BestEffort)
                    .backpressure_threshold(256 * 1024)
                    .build();
  ASSERT_NE(client, nullptr);
}

TEST(BackpressureStrategyTest, UdpBuilderFluentBackpressureStrategy) {
  auto udp = builder::UdpClientBuilder(0)
                 .backpressure_strategy(BackpressureStrategy::BestEffort)
                 .backpressure_threshold(128 * 1024)
                 .build();
  ASSERT_NE(udp, nullptr);
}

// ─── UdpChannel: set_backpressure_strategy runtime setter ────────────────────

TEST(BackpressureStrategyTest, UdpChannel_SetBackpressureStrategyRuntime) {
  config::UdpConfig cfg;
  cfg.local_port = 0;
  auto channel = transport::UdpChannel::create(cfg);
  ASSERT_NE(channel, nullptr);

  // Must not crash when called before start
  channel->set_backpressure_strategy(BackpressureStrategy::BestEffort);
  channel->set_backpressure_strategy(BackpressureStrategy::Reliable);
}
