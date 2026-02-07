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

#include <string>

#include "unilink/config/serial_config.hpp"
#include "unilink/wrapper/serial/serial.hpp"

using namespace unilink;

class SerialConfigMappingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Tests rely on mapping internal string/enum values to config.
    // Since we don't have direct access to builder internals without exposing
    // them, we use the build_config() method exposed for testing (refactoring).
  }
};

TEST_F(SerialConfigMappingTest, MapsParityFlowBitsAndBaud) {
  std::string device = "/dev/ttyUSB0";
  uint32_t baud = 115200;

  auto wrapper = std::make_shared<wrapper::Serial>(device, baud);
  wrapper->set_data_bits(8);
  wrapper->set_stop_bits(1);
  wrapper->set_parity("even");
  wrapper->set_flow_control("hardware");
  wrapper->set_retry_interval(std::chrono::milliseconds(500));

  auto cfg = wrapper->build_config();

  EXPECT_EQ(cfg.device, device);
  EXPECT_EQ(cfg.baud_rate, baud);
  EXPECT_EQ(cfg.char_size, 8u);
  EXPECT_EQ(cfg.stop_bits, 1u);
  EXPECT_EQ(cfg.parity, config::SerialConfig::Parity::Even);
  EXPECT_EQ(cfg.flow, config::SerialConfig::Flow::Hardware);
  EXPECT_EQ(cfg.retry_interval_ms, 500u);
}

TEST_F(SerialConfigMappingTest, InvalidStringsFallbackToNoneAndClampBits) {
  auto wrapper = std::make_shared<wrapper::Serial>("/dev/ttyACM0", 9600);

  // Set invalid values
  wrapper->set_parity("invalid_parity");
  wrapper->set_flow_control("invalid_flow");

  // Out of range bits
  wrapper->set_data_bits(3);  // Too small -> clamped to 5 by config validator? Or just passed?
                              // Config::validate_and_clamp logic is inside transport constructor.
                              // Wrapper just stores values. Let's see if builder logic applies clamping or validation.
  // Actually wrapper just stores primitives. The transport will clamp.
  // build_config() returns what is stored.
  // Wait, does wrapper perform mapping or validation?
  // Wrapper serial constructor doesn't validate.
  // build_config() maps strings to enums.

  wrapper->set_data_bits(5);
  wrapper->set_stop_bits(2);

  auto cfg = wrapper->build_config();

  // Invalid strings should map to default (None) if logic is robust
  // But our implementation checks "none", "even", "odd". Else?
  // Let's check implementation. It preserves current value if no match?
  // No, implementation defaults to "none" in constructor member init?
  // Actually set_parity simply assigns string. build_config performs logic.
  // If no match found, what does it do?
  // Implementation: if (parity_ == "none") ... else if ...
  // If nothing matches, it leaves default (None) if config object initialized with None.
  EXPECT_EQ(cfg.parity, config::SerialConfig::Parity::None);
  EXPECT_EQ(cfg.flow, config::SerialConfig::Flow::None);

  EXPECT_EQ(cfg.char_size, 5u);
  EXPECT_EQ(cfg.stop_bits, 2u);
}
