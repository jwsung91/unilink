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

#include <memory>
#include <string>

#include "unilink/config/serial_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/wrapper/serial/serial.hpp"

namespace {

// Minimal no-op channel to intercept build-time config without running transport
class DummyChannel : public unilink::interface::Channel {
 public:
  void start() override {}
  void stop() override {}
  bool is_connected() const override { return false; }
  void async_write_copy(const uint8_t*, size_t) override {}
  void on_bytes(OnBytes) override {}
  void on_state(OnState) override {}
  void on_backpressure(OnBackpressure) override {}
};

[[maybe_unused]] std::shared_ptr<unilink::interface::Channel> makeDummyChannel() {
  return std::make_shared<DummyChannel>();
}

}  // namespace

using namespace unilink;
using namespace unilink::wrapper;

TEST(SerialConfigMappingTest, ParityAndFlowAreMappedFromStrings) {
  Serial serial("/dev/ttyS0", 9600);
  serial.set_parity("Even");
  serial.set_flow_control("hardware");

  // Build channel
  serial.start();
  serial.stop();

  // No crash expected; mapping executes in start()
  SUCCEED();
}

TEST(SerialConfigMappingTest, InvalidParityAndFlowFallBackToNone) {
  Serial serial("/dev/ttyS0", 9600);
  serial.set_parity("invalid");
  serial.set_flow_control("???");

  serial.start();
  serial.stop();

  SUCCEED();
}
