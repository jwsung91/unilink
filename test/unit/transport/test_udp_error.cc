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
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/transport/udp/udp.hpp"

using namespace unilink;
using namespace unilink::transport;
namespace net = boost::asio;
using namespace std::chrono_literals;

TEST(TransportUdpErrorTest, SendOversizedPacket) {
  net::io_context ioc;

  config::UdpConfig cfg;
  cfg.local_address = "127.0.0.1";
  cfg.local_port = 0;  // Ephemeral
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = 12345;
  cfg.backpressure_threshold = 1024 * 1024;  // 1MB

  auto channel = UdpChannel::create(cfg, ioc);

  std::atomic<bool> error_occurred{false};
  channel->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Error) {
      error_occurred = true;
    }
  });

  channel->start();

  // UDP payload limit is 65535. Sending 70000 should fail.
  // The UdpChannel implementation checks against MAX_BUFFER_SIZE (64MB) and bp_limit (tuned by backpressure_threshold).
  // 70KB is within those limits, so it will attempt async_send_to.
  // async_send_to should fail with message_size error.
  std::vector<uint8_t> huge_packet(70000, 0xDD);
  channel->async_write_copy(huge_packet.data(), huge_packet.size());

  // Run loop to process send
  ioc.run_for(100ms);

  EXPECT_TRUE(error_occurred.load()) << "Sending >65535 bytes via UDP should trigger Error state";

  channel->stop();
}
