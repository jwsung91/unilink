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

#include "unilink/config/udp_config.hpp"
#include "unilink/transport/udp/udp.hpp"
#include "unilink/wrapper/udp/udp_server.hpp"

using namespace unilink;
using namespace std::chrono_literals;

TEST(UdpServerWrapperAdvancedTest, AutoManageStartsInjectedTransport) {
  config::UdpConfig cfg;
  cfg.local_address = "127.0.0.1";
  cfg.local_port = 0;

  boost::asio::io_context ioc;
  auto transport_server = transport::UdpChannel::create(cfg, ioc);
  wrapper::UdpServer server(std::static_pointer_cast<interface::Channel>(transport_server));

  server.auto_manage(true);
  ioc.run_for(50ms);

  EXPECT_TRUE(server.is_listening());

  server.stop();
}

TEST(UdpServerWrapperAdvancedTest, StartFutureReflectsBindFailure) {
  boost::asio::io_context guard_ioc;
  boost::asio::ip::udp::socket occupied_socket(guard_ioc);
  occupied_socket.open(boost::asio::ip::udp::v4());
  occupied_socket.bind({boost::asio::ip::make_address("127.0.0.1"), 0});

  config::UdpConfig cfg;
  cfg.local_address = "127.0.0.1";
  cfg.local_port = occupied_socket.local_endpoint().port();

  wrapper::UdpServer server(cfg);
  auto started = server.start();

  ASSERT_EQ(started.wait_for(2s), std::future_status::ready);
  EXPECT_FALSE(started.get());
  EXPECT_FALSE(server.is_listening());

  server.stop();
}
