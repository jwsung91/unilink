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
#include <string>
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/transport/udp/udp.hpp"

using namespace std::chrono_literals;
using namespace unilink;

namespace {
uint16_t next_port() {
  static std::atomic<uint16_t> base{28000};
  return base.fetch_add(5);
}

template <typename Pred>
bool wait_for_condition(boost::asio::io_context& ioc, Pred pred, std::chrono::milliseconds timeout,
                        std::chrono::milliseconds step = 5ms) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) return true;
    ioc.run_for(step);
    ioc.restart();
  }
  return pred();
}

void pump_io(boost::asio::io_context& ioc, std::chrono::milliseconds duration, std::chrono::milliseconds step = 5ms) {
  auto deadline = std::chrono::steady_clock::now() + duration;
  while (std::chrono::steady_clock::now() < deadline) {
    ioc.run_for(step);
    ioc.restart();
  }
}
}  // namespace

TEST(TransportUdpExtendedTest, AsyncWriteMove) {
  boost::asio::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = next_port();
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = cfg.local_port + 1;

  auto channel = transport::UdpChannel::create(cfg, ioc);

  // Setup receiver
  boost::asio::ip::udp::socket rx_socket(ioc, {boost::asio::ip::udp::v4(), cfg.remote_port.value()});
  std::vector<uint8_t> rx_buf(1024);
  size_t received_bytes = 0;
  boost::asio::ip::udp::endpoint sender_ep;
  rx_socket.async_receive_from(boost::asio::buffer(rx_buf), sender_ep,
                               [&](auto ec, auto n) { if (!ec) received_bytes = n; });

  channel->start();

  std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
  size_t payload_size = payload.size();
  channel->async_write_move(std::move(payload));

  EXPECT_TRUE(wait_for_condition(ioc, [&] { return received_bytes == payload_size; }, 200ms));

  channel->stop();
}

TEST(TransportUdpExtendedTest, AsyncWriteShared) {
  boost::asio::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = next_port();
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = cfg.local_port + 1;

  auto channel = transport::UdpChannel::create(cfg, ioc);

  // Setup receiver
  boost::asio::ip::udp::socket rx_socket(ioc, {boost::asio::ip::udp::v4(), cfg.remote_port.value()});
  std::vector<uint8_t> rx_buf(1024);
  size_t received_bytes = 0;
  boost::asio::ip::udp::endpoint sender_ep;
  rx_socket.async_receive_from(boost::asio::buffer(rx_buf), sender_ep,
                               [&](auto ec, auto n) { if (!ec) received_bytes = n; });

  channel->start();

  auto payload = std::make_shared<std::vector<uint8_t>>(std::initializer_list<uint8_t>{0xAA, 0xBB});
  size_t payload_size = payload->size();
  channel->async_write_shared(payload);

  EXPECT_TRUE(wait_for_condition(ioc, [&] { return received_bytes == payload_size; }, 200ms));

  channel->stop();
}

TEST(TransportUdpExtendedTest, PooledBufferWrite) {
  boost::asio::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = next_port();
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = cfg.local_port + 1;
  cfg.enable_memory_pool = true;

  auto channel = transport::UdpChannel::create(cfg, ioc);

  // Setup receiver
  boost::asio::ip::udp::socket rx_socket(ioc, {boost::asio::ip::udp::v4(), cfg.remote_port.value()});
  std::vector<uint8_t> rx_buf(1024);
  size_t received_bytes = 0;
  boost::asio::ip::udp::endpoint sender_ep;
  rx_socket.async_receive_from(boost::asio::buffer(rx_buf), sender_ep,
                               [&](auto ec, auto n) { if (!ec) received_bytes = n; });

  channel->start();

  // Use a size that fits in small pool buckets
  std::vector<uint8_t> payload(100, 0xCC);
  channel->async_write_copy(payload.data(), payload.size());

  EXPECT_TRUE(wait_for_condition(ioc, [&] { return received_bytes == payload.size(); }, 200ms));

  channel->stop();
}

TEST(TransportUdpExtendedTest, BackpressureReporting) {
  boost::asio::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = next_port();
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = cfg.local_port + 1; // Send to something that might not read fast enough?
  // Actually, we rely on queue filling up inside the channel before socket sends.
  cfg.backpressure_threshold = 100;

  auto channel = transport::UdpChannel::create(cfg, ioc);

  std::atomic<bool> bp_triggered{false};
  std::atomic<bool> bp_cleared{false};

  channel->on_backpressure([&](size_t q) {
      if (q >= 100) bp_triggered = true;
      if (q == 0 && bp_triggered) bp_cleared = true;
  });

  channel->start();

  // We don't run the IOC yet, so writes should queue up
  // MIN_BACKPRESSURE_THRESHOLD is 1024, so we need > 1024 bytes to trigger it.
  std::vector<uint8_t> chunk(800, 0xFF);

  // First write: 800 bytes (below threshold 1024)
  channel->async_write_copy(chunk.data(), chunk.size());

  // Second write: 1600 bytes total (above threshold)
  channel->async_write_copy(chunk.data(), chunk.size());

  // Now run IO to drain. The strand ensures the writes are processed
  // before the send completion handler reduces the queue, triggering the BP callback.
  pump_io(ioc, 50ms);

  EXPECT_TRUE(bp_triggered);
  EXPECT_TRUE(bp_cleared);

  channel->stop();
}

TEST(TransportUdpExtendedTest, CallbackExceptionSafety) {
    boost::asio::io_context ioc;
    config::UdpConfig cfg;
    cfg.local_port = next_port();
    cfg.stop_on_callback_exception = false; // Should not stop on exception

    auto channel = transport::UdpChannel::create(cfg, ioc);

    std::atomic<int> calls{0};
    channel->on_state([&](base::LinkState) {
        calls++;
        throw std::runtime_error("State boom");
    });

    channel->start();

    // Trigger state change
    pump_io(ioc, 10ms);

    // Should still be running/usable
    EXPECT_GT(calls, 0);
    EXPECT_NO_THROW(channel->stop());
}
