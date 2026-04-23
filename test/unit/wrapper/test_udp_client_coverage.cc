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

#include "unilink/framer/line_framer.hpp"
#include "unilink/wrapper/udp/udp.hpp"

using namespace unilink;

namespace unilink {
namespace test {

TEST(UdpClientCoverageTest, StartMultipleTimes) {
  config::UdpConfig cfg;
  cfg.local_port = 0;
  wrapper::UdpClient client(cfg);

  (void)client.start();
  (void)client.start();
}

TEST(UdpClientCoverageTest, ExternalIoContextManagement) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  config::UdpConfig cfg;
  cfg.local_port = 0;

  wrapper::UdpClient client(cfg, ioc);
  client.manage_external_context(true);

  (void)client.start();
  EXPECT_FALSE(ioc->stopped());

  client.stop();
  EXPECT_TRUE(ioc->stopped());
}

TEST(UdpClientCoverageTest, SendWithoutConnection) {
  config::UdpConfig cfg;
  wrapper::UdpClient client(cfg);
  EXPECT_FALSE(client.send("data"));
  EXPECT_FALSE(client.send_line("line"));
}

TEST(UdpClientCoverageTest, FramerIntegration) {
  config::UdpConfig cfg;
  wrapper::UdpClient client(cfg);
  client.framer(std::make_unique<framer::LineFramer>());
  client.on_message([](const wrapper::MessageContext&) {});
}

TEST(UdpClientCoverageTest, StopWithoutStart) {
  config::UdpConfig cfg;
  wrapper::UdpClient client(cfg);
  client.stop();
}

}  // namespace test
}  // namespace unilink
