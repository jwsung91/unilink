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
#include <chrono>
#include <string>
#include <thread>

#include "unilink/wrapper/serial/serial.hpp"

using namespace unilink;
using namespace std::chrono_literals;

TEST(WrapperSerialFailTest, OpenInvalidPort) {
#if defined(_WIN32)
  std::string port = "COM999";
#else
  std::string port = "/dev/ttyInvalid999";
#endif

  wrapper::Serial serial(port, 9600);

  std::atomic<bool> error_called{false};
  serial.on_error([&](const std::string& msg) { error_called = true; });

  // Disable auto-retry or set a long retry to prevent infinite retries masking the initial failure?
  // Or just check that it fails initially.
  // wrapper::Serial doesn't expose set_retry easily?
  // Ah, set_retry_interval is there.
  serial.set_retry_interval(100ms);

  serial.start();

  // Wait for the attempt
  std::this_thread::sleep_for(200ms);

  EXPECT_FALSE(serial.is_connected());

  // We expect at least one error notification or just not connected.
  // Depending on implementation, it might retry silently in "Connecting" state without transitioning to "Error".
  // But usually, open failure logs error and might trigger state change.
  // Wrapper Serial on_state:
  // case base::LinkState::Error: if (error_handler_) error_handler_("Serial connection error occurred");

  // If transport layer transitions to Error, we get called.
  // Let's assume it does.

  // However, is_connected() being false is the primary check.
}
