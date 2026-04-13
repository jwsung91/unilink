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

#include <boost/asio.hpp>
#include <chrono>
#include <thread>

#include "test_utils.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/wrapper/udp/udp.hpp"

using namespace unilink::wrapper;
using namespace unilink::config;
using namespace unilink::test;

namespace {

class UdpStateTest : public ::testing::Test {};

TEST_F(UdpStateTest, BindConflict) {
  using namespace std::chrono_literals;

  boost::asio::io_context guard_ioc;
  boost::asio::ip::udp::socket occupied_socket(guard_ioc);
  occupied_socket.open(boost::asio::ip::udp::v4());
  occupied_socket.bind({boost::asio::ip::make_address("127.0.0.1"), 0});

  const uint16_t port = occupied_socket.local_endpoint().port();

  UdpConfig cfg2;
  cfg2.local_address = "127.0.0.1";
  cfg2.local_port = port;  // Same port already occupied by a live UDP socket
  Udp udp2(cfg2);

  auto udp2_started = udp2.start();
  ASSERT_EQ(udp2_started.wait_for(2s), std::future_status::ready);
  EXPECT_FALSE(udp2_started.get());

  // Verify it did not successfully start/connect
  EXPECT_FALSE(udp2.is_connected());

  udp2.stop();
}

TEST_F(UdpStateTest, UninitializedUse) {
  UdpConfig cfg;
  cfg.local_port = 0;
  Udp udp(cfg);

  // Object created but not started (uninitialized state)
  EXPECT_FALSE(udp.is_connected());

  // Try to call send()
  // Should handle gracefully (no crash, likely no-op or log error)
  EXPECT_NO_THROW(udp.send("test data"));

  // Try to call send_line()
  EXPECT_NO_THROW(udp.send_line("test line"));
}

}  // namespace
