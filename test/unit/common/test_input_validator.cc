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
#include <vector>

#include "unilink/diagnostics/exceptions.hpp"
#include "unilink/common/input_validator.hpp"

using namespace unilink::common;

/**
 * @brief Input Validator Coverage Test
 */
class InputValidatorTest : public ::testing::Test {};

// ============================================================================
// NETWORK VALIDATION
// ============================================================================

TEST_F(InputValidatorTest, ValidatePortValid) {
  EXPECT_NO_THROW(InputValidator::validate_port(80));
  EXPECT_NO_THROW(InputValidator::validate_port(443));
  EXPECT_NO_THROW(InputValidator::validate_port(8080));
  EXPECT_NO_THROW(InputValidator::validate_port(65535));
}

TEST_F(InputValidatorTest, ValidatePortInvalid) { EXPECT_THROW(InputValidator::validate_port(0), ValidationException); }

TEST_F(InputValidatorTest, ValidateHostValid) {
  EXPECT_NO_THROW(InputValidator::validate_host("localhost"));
  EXPECT_NO_THROW(InputValidator::validate_host("example.com"));
  EXPECT_NO_THROW(InputValidator::validate_host("test-server.local"));
  EXPECT_NO_THROW(InputValidator::validate_host("127.0.0.1"));
  EXPECT_NO_THROW(InputValidator::validate_host("192.168.1.1"));
  EXPECT_NO_THROW(InputValidator::validate_host("::1"));
}

TEST_F(InputValidatorTest, ValidateHostInvalid) {
  EXPECT_THROW(InputValidator::validate_host(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_host("invalid host!@#"), ValidationException);
}

TEST_F(InputValidatorTest, ValidateIPv4Valid) {
  EXPECT_NO_THROW(InputValidator::validate_ipv4_address("0.0.0.0"));
  EXPECT_NO_THROW(InputValidator::validate_ipv4_address("127.0.0.1"));
  EXPECT_NO_THROW(InputValidator::validate_ipv4_address("192.168.1.1"));
  EXPECT_NO_THROW(InputValidator::validate_ipv4_address("255.255.255.255"));
}

TEST_F(InputValidatorTest, ValidateIPv4Invalid) {
  EXPECT_THROW(InputValidator::validate_ipv4_address(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv4_address("256.1.1.1"), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.1.1"), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv4_address("invalid"), ValidationException);
}

TEST_F(InputValidatorTest, ValidateIPv6Valid) {
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("::1"));
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("2001:db8:0:0:0:0:0:1"));
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("fe80:0:0:0:0:0:0:1"));
}

TEST_F(InputValidatorTest, ValidateIPv6Invalid) {
  EXPECT_THROW(InputValidator::validate_ipv6_address(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv6_address("invalid"), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv6_address("192.168.1.1"), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv6_address("gggg::1"), ValidationException);
}

// ============================================================================
// SERIAL VALIDATION
// ============================================================================

TEST_F(InputValidatorTest, ValidateDevicePathValid) {
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyUSB0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyS0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyACM0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("COM1"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("COM10"));
}

TEST_F(InputValidatorTest, ValidateDevicePathInvalid) {
  EXPECT_THROW(InputValidator::validate_device_path(""), ValidationException);
}

TEST_F(InputValidatorTest, ValidateBaudRateValid) {
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(50));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(9600));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(115200));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(4000000));
}

TEST_F(InputValidatorTest, ValidateBaudRateInvalid) {
  EXPECT_THROW(InputValidator::validate_baud_rate(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_baud_rate(49), ValidationException);
  EXPECT_THROW(InputValidator::validate_baud_rate(5000000), ValidationException);
}

TEST_F(InputValidatorTest, ValidateDataBitsValid) {
  EXPECT_NO_THROW(InputValidator::validate_data_bits(5));
  EXPECT_NO_THROW(InputValidator::validate_data_bits(6));
  EXPECT_NO_THROW(InputValidator::validate_data_bits(7));
  EXPECT_NO_THROW(InputValidator::validate_data_bits(8));
}

TEST_F(InputValidatorTest, ValidateDataBitsInvalid) {
  EXPECT_THROW(InputValidator::validate_data_bits(4), ValidationException);
  EXPECT_THROW(InputValidator::validate_data_bits(9), ValidationException);
}

TEST_F(InputValidatorTest, ValidateStopBitsValid) {
  EXPECT_NO_THROW(InputValidator::validate_stop_bits(1));
  EXPECT_NO_THROW(InputValidator::validate_stop_bits(2));
}

TEST_F(InputValidatorTest, ValidateStopBitsInvalid) {
  EXPECT_THROW(InputValidator::validate_stop_bits(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_stop_bits(3), ValidationException);
}

TEST_F(InputValidatorTest, ValidateParityValid) {
  EXPECT_NO_THROW(InputValidator::validate_parity("none"));
  EXPECT_NO_THROW(InputValidator::validate_parity("odd"));
  EXPECT_NO_THROW(InputValidator::validate_parity("even"));
  EXPECT_NO_THROW(InputValidator::validate_parity("NONE"));
  EXPECT_NO_THROW(InputValidator::validate_parity("ODD"));
  EXPECT_NO_THROW(InputValidator::validate_parity("EVEN"));
}

TEST_F(InputValidatorTest, ValidateParityInvalid) {
  EXPECT_THROW(InputValidator::validate_parity(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_parity("invalid"), ValidationException);
  EXPECT_THROW(InputValidator::validate_parity("mark"), ValidationException);
}

// ============================================================================
// MEMORY VALIDATION
// ============================================================================

TEST_F(InputValidatorTest, ValidateBufferSizeValid) {
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(1));
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(1024));
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(1024 * 1024));
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(64 * 1024 * 1024));
}

TEST_F(InputValidatorTest, ValidateBufferSizeInvalid) {
  EXPECT_THROW(InputValidator::validate_buffer_size(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_buffer_size(65 * 1024 * 1024), ValidationException);
}

TEST_F(InputValidatorTest, ValidateMemoryAlignmentValid) {
  alignas(16) char buffer[64];
  EXPECT_NO_THROW(InputValidator::validate_memory_alignment(buffer, 1));
  EXPECT_NO_THROW(InputValidator::validate_memory_alignment(buffer, 2));
  EXPECT_NO_THROW(InputValidator::validate_memory_alignment(buffer, 4));
  EXPECT_NO_THROW(InputValidator::validate_memory_alignment(buffer, 8));
  EXPECT_NO_THROW(InputValidator::validate_memory_alignment(buffer, 16));
}

TEST_F(InputValidatorTest, ValidateMemoryAlignmentInvalid) {
  EXPECT_THROW(InputValidator::validate_memory_alignment(nullptr, 4), ValidationException);

  char buffer[64];
  char* misaligned = buffer + 1;
  EXPECT_THROW(InputValidator::validate_memory_alignment(misaligned, 2), ValidationException);
}

// ============================================================================
// TIMEOUT AND RETRY VALIDATION
// ============================================================================

TEST_F(InputValidatorTest, ValidateTimeoutValid) {
  EXPECT_NO_THROW(InputValidator::validate_timeout(100));
  EXPECT_NO_THROW(InputValidator::validate_timeout(1000));
  EXPECT_NO_THROW(InputValidator::validate_timeout(5000));
  EXPECT_NO_THROW(InputValidator::validate_timeout(300000));
}

TEST_F(InputValidatorTest, ValidateTimeoutInvalid) {
  EXPECT_THROW(InputValidator::validate_timeout(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_timeout(99), ValidationException);
  EXPECT_THROW(InputValidator::validate_timeout(300001), ValidationException);
}

TEST_F(InputValidatorTest, ValidateRetryIntervalValid) {
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(100));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(1000));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(2000));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(60000));
}

TEST_F(InputValidatorTest, ValidateRetryIntervalInvalid) {
  EXPECT_THROW(InputValidator::validate_retry_interval(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_interval(99), ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_interval(300001), ValidationException);
}

TEST_F(InputValidatorTest, ValidateRetryCountValid) {
  EXPECT_NO_THROW(InputValidator::validate_retry_count(0));
  EXPECT_NO_THROW(InputValidator::validate_retry_count(1));
  EXPECT_NO_THROW(InputValidator::validate_retry_count(10));
  EXPECT_NO_THROW(InputValidator::validate_retry_count(100));
}

TEST_F(InputValidatorTest, ValidateRetryCountInvalid) {
  EXPECT_THROW(InputValidator::validate_retry_count(-1), ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_count(101), ValidationException);
}

// ============================================================================
// STRING VALIDATION
// ============================================================================

TEST_F(InputValidatorTest, ValidateNonEmptyStringValid) {
  EXPECT_NO_THROW(InputValidator::validate_non_empty_string("test", "field"));
  EXPECT_NO_THROW(InputValidator::validate_non_empty_string("a", "field"));
}

TEST_F(InputValidatorTest, ValidateNonEmptyStringInvalid) {
  EXPECT_THROW(InputValidator::validate_non_empty_string("", "field"), ValidationException);
}

TEST_F(InputValidatorTest, ValidateStringLengthValid) {
  EXPECT_NO_THROW(InputValidator::validate_string_length("test", 10, "field"));
  EXPECT_NO_THROW(InputValidator::validate_string_length("", 10, "field"));
  EXPECT_NO_THROW(InputValidator::validate_string_length("1234567890", 10, "field"));
}

TEST_F(InputValidatorTest, ValidateStringLengthInvalid) {
  EXPECT_THROW(InputValidator::validate_string_length("12345678901", 10, "field"), ValidationException);
}

// ============================================================================
// NUMERIC VALIDATION
// ============================================================================

TEST_F(InputValidatorTest, ValidatePositiveNumberValid) {
  EXPECT_NO_THROW(InputValidator::validate_positive_number(1, "field"));
  EXPECT_NO_THROW(InputValidator::validate_positive_number(100, "field"));
  EXPECT_NO_THROW(InputValidator::validate_positive_number(9999, "field"));
}

TEST_F(InputValidatorTest, ValidatePositiveNumberInvalid) {
  EXPECT_THROW(InputValidator::validate_positive_number(0, "field"), ValidationException);
  EXPECT_THROW(InputValidator::validate_positive_number(-1, "field"), ValidationException);
}

TEST_F(InputValidatorTest, ValidateRangeInt64Valid) {
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<int64_t>(0), 0, 100, "field"));
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<int64_t>(50), 0, 100, "field"));
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<int64_t>(100), 0, 100, "field"));
}

TEST_F(InputValidatorTest, ValidateRangeInt64Invalid) {
  EXPECT_THROW(InputValidator::validate_range(static_cast<int64_t>(-1), 0, 100, "field"), ValidationException);
  EXPECT_THROW(InputValidator::validate_range(static_cast<int64_t>(101), 0, 100, "field"), ValidationException);
}

TEST_F(InputValidatorTest, ValidateRangeSizeTValid) {
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<size_t>(0), 0, 100, "field"));
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<size_t>(50), 0, 100, "field"));
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<size_t>(100), 0, 100, "field"));
}

TEST_F(InputValidatorTest, ValidateRangeSizeTInvalid) {
  EXPECT_THROW(InputValidator::validate_range(static_cast<size_t>(101), 0, 100, "field"), ValidationException);
}
