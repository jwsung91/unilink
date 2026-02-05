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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;

class SerialLoopbackTest : public ::testing::Test {
 protected:
  void SetUp() override {
#ifdef __linux__
    // Start socat to create virtual serial ports
    // -d -d prints fatal, error, warning, notice messages
    // pty,raw,echo=0: create pseudo-terminal, raw mode (no processing), no echo
    // link=/tmp/ttyV0: create symlink
    // & runs in background
    // We check availability of socat first
    int check = std::system("which socat > /dev/null 2>&1");
    if (check != 0) {
      std::cout << "socat not found, skipping socat setup" << std::endl;
      socat_available_ = false;
      return;
    }

    // cleanup previous runs just in case
    std::system("pkill -f 'socat -d -d pty,raw,echo=0,link=/tmp/ttyV0'");

    int ret = std::system(
        "socat -d -d pty,raw,echo=0,link=/tmp/ttyV0 pty,raw,echo=0,link=/tmp/ttyV1 > /dev/null 2>&1 &");
    ASSERT_EQ(ret, 0) << "Failed to start socat";

    // Wait for ports to be created
    TestUtils::waitFor(500);
#endif
  }

  void TearDown() override {
#ifdef __linux__
    if (socat_available_) {
      // Kill socat
      std::system("pkill -f 'socat -d -d pty,raw,echo=0,link=/tmp/ttyV0'");
      // Clean up symlinks
      std::remove("/tmp/ttyV0");
      std::remove("/tmp/ttyV1");
    }
#endif
  }

  bool socat_available_ = true;
};

TEST_F(SerialLoopbackTest, LoopbackCommunication) {
#ifndef __linux__
  GTEST_SKIP() << "Skipping SerialLoopbackTest on non-Linux platform";
#else
  if (!socat_available_) {
      GTEST_SKIP() << "socat not available";
  }

  // Check if ports exist
  if (!std::filesystem::exists("/tmp/ttyV0") || !std::filesystem::exists("/tmp/ttyV1")) {
    GTEST_SKIP() << "Virtual serial ports not found (socat failed?)";
  }

  std::string received_data;
  std::atomic<bool> data_received{false};

  // Open port V0 with Unilink::Serial
  // Note: 9600 is arbitrary for virtual ports
  // We use shared_ptr to manage lifecycle
  auto serial1 = std::make_shared<wrapper::Serial>("/tmp/ttyV0", 9600);
  auto serial2 = std::make_shared<wrapper::Serial>("/tmp/ttyV1", 9600);

  serial2->on_data([&](const std::string& data) {
    received_data += data;
    data_received = true;
  });

  serial1->start();
  serial2->start();
  TestUtils::waitFor(100);

  // Write to V0
  std::string test_msg = "Hello Serial";
  serial1->send(test_msg);

  // Verify reception on V1
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return data_received.load(); }, 2000));
  EXPECT_EQ(received_data, test_msg);

  // Stop
  serial1->stop();
  serial2->stop();
#endif
}
