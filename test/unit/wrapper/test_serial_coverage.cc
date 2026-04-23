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
#include "unilink/wrapper/serial/serial.hpp"

using namespace unilink;
using namespace std::chrono_literals;

namespace unilink {
namespace test {

TEST(SerialCoverageTest, StartMultipleTimes) {
  wrapper::Serial s("/dev/null", 9600);
  (void)s.start();
  (void)s.start();
}

TEST(SerialCoverageTest, ExternalIoContextManagement) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  wrapper::Serial s("/dev/null", 9600, ioc);
  s.manage_external_context(true);
  (void)s.start();
  EXPECT_FALSE(ioc->stopped());
  s.stop();
  EXPECT_TRUE(ioc->stopped());
}

TEST(SerialCoverageTest, ConfigurationParsing) {
  wrapper::Serial s("/dev/null", 115200);
  s.parity("even").flow_control("hardware").data_bits(7).stop_bits(2).retry_interval(100ms);

  // parity/flow/etc are parsed inside build_config() which is called on start()
  // or manually. Since it is protected, we trigger it via start() indirectly
  // if we could. But for now we just exercise the setters for line coverage.
  s.parity("odd").flow_control("software").parity("none").flow_control("none");
}

TEST(SerialCoverageTest, FramerIntegration) {
  wrapper::Serial s("/dev/null", 9600);
  s.framer(std::make_unique<framer::LineFramer>());
  s.on_message([](const wrapper::MessageContext&) {});
}

TEST(SerialCoverageTest, SendWithoutConnection) {
  wrapper::Serial s("/dev/null", 9600);
  EXPECT_FALSE(s.send("data"));
  EXPECT_FALSE(s.send_line("line"));
}

TEST(SerialCoverageTest, StopWithoutStart) {
  wrapper::Serial s("/dev/null", 9600);
  s.stop();
}

}  // namespace test
}  // namespace unilink
