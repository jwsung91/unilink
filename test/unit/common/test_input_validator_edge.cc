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

#include "unilink/util/input_validator.hpp"

using namespace unilink::util;
using namespace unilink::diagnostics;

TEST(InputValidatorEdgeTest, DevicePathEdges) {
  // Path not starting with / and not COM/Special (e.g. relative path or custom
  // name)
  EXPECT_THROW(InputValidator::validate_device_path("custom_device"),
               ValidationException);
  EXPECT_THROW(InputValidator::validate_device_path("./dev/ttyUSB0"),
               ValidationException);

  // Valid complex linux path
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyUSB-1_2"));
}

TEST(InputValidatorEdgeTest, HostnameEdges) {
  // Invalid char in label
  EXPECT_THROW(InputValidator::validate_host("example.c$om"),
               ValidationException);
  EXPECT_THROW(InputValidator::validate_host("ex ample.com"),
               ValidationException);
}

TEST(InputValidatorEdgeTest, IPv6Edges) {
  // Boundary cases for regex
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("::"));
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("::1"));
}

TEST(InputValidatorEdgeTest, RangeEdges) {
  // Test range boundaries explicitly
  EXPECT_NO_THROW(InputValidator::validate_range(
      static_cast<int64_t>(10), static_cast<int64_t>(10),
      static_cast<int64_t>(20), "min_boundary"));
  EXPECT_NO_THROW(InputValidator::validate_range(
      static_cast<int64_t>(20), static_cast<int64_t>(10),
      static_cast<int64_t>(20), "max_boundary"));

  // Zero range
  EXPECT_NO_THROW(InputValidator::validate_range(
      static_cast<int64_t>(5), static_cast<int64_t>(5), static_cast<int64_t>(5),
      "zero_range"));
  EXPECT_THROW(InputValidator::validate_range(
                   static_cast<int64_t>(4), static_cast<int64_t>(5),
                   static_cast<int64_t>(5), "zero_range_low"),
               ValidationException);
}

TEST(InputValidatorEdgeTest, PositiveNumberEdges) {
  // Max int64
  EXPECT_NO_THROW(InputValidator::validate_positive_number(
      std::numeric_limits<int64_t>::max(), "max_int64"));
}
