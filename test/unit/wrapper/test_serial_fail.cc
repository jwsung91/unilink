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
#include <memory>
#include <string>
#include <thread>

#include "unilink/unilink.hpp"

using namespace unilink;

// Test wrapper behavior when open fails
TEST(WrapperSerialFailTest, OpenInvalidPort) {
  // Use a device name that is guaranteed to not exist or fail
  std::string invalid_device = "/dev/non_existent_device_unilink_test";
#ifdef _WIN32
  invalid_device = "COM999";
#endif

  wrapper::Serial serial(invalid_device, 9600);
  std::atomic<bool> error_called{false};

  serial.on_error([&](const wrapper::ErrorContext& err) { error_called = true; });

  // Try to start - should trigger error or at least not be connected
  serial.start();

  // Wait a bit for async operations
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(serial.is_connected());
  // Note: whether error callback is called depends on implementation details
  // but it should not crash.

  serial.stop();
}
