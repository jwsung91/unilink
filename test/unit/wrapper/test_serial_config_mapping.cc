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
using namespace unilink::wrapper;

TEST(SerialConfigMappingTest, MapsParityFlowBitsAndBaud) {
  Serial wrapper("/dev/ttyS10", 57600);
  wrapper.set_data_bits(7);
  wrapper.set_stop_bits(2);
  wrapper.set_parity("Even");
  wrapper.set_flow_control("hardware");
  wrapper.set_retry_interval(std::chrono::milliseconds(1500));

  auto cfg = wrapper.build_config();

  EXPECT_EQ(cfg.device, "/dev/ttyS10");
  EXPECT_EQ(cfg.baud_rate, 57600u);
  EXPECT_EQ(cfg.char_size, 7u);
  EXPECT_EQ(cfg.stop_bits, 2u);
  EXPECT_EQ(cfg.retry_interval_ms, 1500u);
  EXPECT_EQ(cfg.parity, config::SerialConfig::Parity::Even);
  EXPECT_EQ(cfg.flow, config::SerialConfig::Flow::Hardware);
}

TEST(SerialConfigMappingTest, InvalidStringsFallbackToNoneAndClampBits) {
  Serial wrapper("/dev/ttyS11", 9600);
  wrapper.set_data_bits(3);    // below minimum, should clamp to 5
  wrapper.set_stop_bits(5);    // above maximum, should clamp to 2
  wrapper.set_parity("invalid");
  wrapper.set_flow_control("???");

  auto cfg = wrapper.build_config();

  EXPECT_EQ(cfg.char_size, 5u);
  EXPECT_EQ(cfg.stop_bits, 2u);
  EXPECT_EQ(cfg.parity, config::SerialConfig::Parity::None);
  EXPECT_EQ(cfg.flow, config::SerialConfig::Flow::None);
}
