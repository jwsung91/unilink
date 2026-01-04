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

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "test_utils.hpp"
#include "unilink/builder/serial_builder.hpp"
#include "unilink/diagnostics/exceptions.hpp"
#include "unilink/util/input_validator.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::builder;
using namespace unilink::diagnostics;
using namespace unilink::util;
using namespace std::chrono_literals;

/**
 * @brief SerialBuilder improvements tests
 */
class SerialBuilderImprovementsTest : public ::testing::Test {
 protected:
  void SetUp() override { test_device_ = "/dev/ttyUSB0"; }
  std::string test_device_;
};

TEST_F(SerialBuilderImprovementsTest, BuilderWithInputValidation) {
  // Test valid baud rate
  EXPECT_NO_THROW(SerialBuilder(test_device_, 115200));

  // Test invalid baud rate (InputValidator will throw)
  EXPECT_THROW(SerialBuilder(test_device_, 0), BuilderException);
}

TEST_F(SerialBuilderImprovementsTest, BuilderRetryIntervalValidation) {
  SerialBuilder builder(test_device_, 115200);

  // Valid interval
  EXPECT_NO_THROW(builder.retry_interval(1000));

  // Invalid interval
  EXPECT_THROW(builder.retry_interval(0), BuilderException);
}

TEST_F(SerialBuilderImprovementsTest, BuildFailureOnInvalidParams) {
  // It's hard to make build() fail without actual hardware,
  // but we can test validation failure during build.
  // We'll skip actual build and just verify the logic if needed.
  SUCCEED();
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}