/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-20.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <limits>

#include "unilink/util/input_validator.hpp"

using namespace unilink;
using namespace unilink::util;

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

#include <limits>
#include <stdexcept>  // For std::invalid_argument if needed (but now using ValidationException)

#include "unilink/base/constants.hpp"  // For MIN/MAX constants
#include "unilink/util/input_validator.hpp"

using namespace unilink;
using namespace unilink::util;

// Using a single test suite for organization
TEST(InputValidatorTest, ValidatePort) {
  EXPECT_NO_THROW(InputValidator::validate_port(1));
  EXPECT_NO_THROW(InputValidator::validate_port(65535));
  EXPECT_THROW(InputValidator::validate_port(0), diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_port(65536), diagnostics::ValidationException);  // uint16_t overflow to 0
  // Negative values like -1, when cast to uint16_t, become large positive.
  // The current validate_port only checks for 0, so -1 will pass as 65535.
  // This is inherent to uint16_t parameter type.
}

TEST(InputValidatorTest, ValidateHost) {
  EXPECT_NO_THROW(InputValidator::validate_host("localhost"));
  EXPECT_NO_THROW(InputValidator::validate_host("127.0.0.1"));
  EXPECT_NO_THROW(InputValidator::validate_host("example.com"));
  EXPECT_THROW(InputValidator::validate_host(""), diagnostics::ValidationException);
}

TEST(InputValidatorTest, ValidateProtocol) {
  // IPv4
  EXPECT_NO_THROW(InputValidator::validate_ipv4_address("1.1.1.1"));
  EXPECT_NO_THROW(InputValidator::validate_ipv4_address("255.255.255.255"));
  EXPECT_THROW(InputValidator::validate_ipv4_address("256.2.3.4"), diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.2.3"), diagnostics::ValidationException);  // Incomplete
  EXPECT_THROW(InputValidator::validate_ipv4_address("invalid"), diagnostics::ValidationException);

  // IPv6
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("::1"));
  EXPECT_NO_THROW(InputValidator::validate_ipv6_address("2001:0db8:85a3:0000:0000:8a2e:0370:7334"));
  EXPECT_THROW(InputValidator::validate_ipv6_address("1.1.1.1"),
               diagnostics::ValidationException);  // Not a valid IPv6 format
  EXPECT_THROW(InputValidator::validate_ipv6_address("invalid"), diagnostics::ValidationException);
}

TEST(InputValidatorTest, ValidateSerialParams) {
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyUSB0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("COM1"));
  EXPECT_THROW(InputValidator::validate_device_path(""), diagnostics::ValidationException);

  EXPECT_NO_THROW(InputValidator::validate_baud_rate(base::constants::MIN_BAUD_RATE));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(base::constants::MAX_BAUD_RATE));
  EXPECT_THROW(InputValidator::validate_baud_rate(base::constants::MIN_BAUD_RATE - 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_baud_rate(base::constants::MAX_BAUD_RATE + 1),
               diagnostics::ValidationException);

  EXPECT_NO_THROW(InputValidator::validate_data_bits(base::constants::MIN_DATA_BITS));
  EXPECT_NO_THROW(InputValidator::validate_data_bits(base::constants::MAX_DATA_BITS));
  EXPECT_THROW(InputValidator::validate_data_bits(base::constants::MIN_DATA_BITS - 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_data_bits(base::constants::MAX_DATA_BITS + 1),
               diagnostics::ValidationException);

  EXPECT_NO_THROW(InputValidator::validate_stop_bits(base::constants::MIN_STOP_BITS));
  EXPECT_NO_THROW(InputValidator::validate_stop_bits(base::constants::MAX_STOP_BITS));
  EXPECT_THROW(InputValidator::validate_stop_bits(base::constants::MIN_STOP_BITS - 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_stop_bits(base::constants::MAX_STOP_BITS + 1),
               diagnostics::ValidationException);
}

TEST(InputValidatorTest, ValidateCommonParams) {
  // Buffer Size
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(base::constants::MIN_BUFFER_SIZE));
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(base::constants::MAX_BUFFER_SIZE));
  EXPECT_THROW(InputValidator::validate_buffer_size(base::constants::MIN_BUFFER_SIZE - 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_buffer_size(base::constants::MAX_BUFFER_SIZE + 1),
               diagnostics::ValidationException);

  // Timeout
  EXPECT_NO_THROW(InputValidator::validate_timeout(base::constants::MIN_CONNECTION_TIMEOUT_MS));
  EXPECT_NO_THROW(InputValidator::validate_timeout(base::constants::MAX_CONNECTION_TIMEOUT_MS));
  EXPECT_THROW(InputValidator::validate_timeout(base::constants::MIN_CONNECTION_TIMEOUT_MS - 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_timeout(base::constants::MAX_CONNECTION_TIMEOUT_MS + 1),
               diagnostics::ValidationException);

  // Retry Interval
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(base::constants::MIN_RETRY_INTERVAL_MS));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(base::constants::MAX_RETRY_INTERVAL_MS));
  EXPECT_THROW(InputValidator::validate_retry_interval(base::constants::MIN_RETRY_INTERVAL_MS - 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_interval(base::constants::MAX_RETRY_INTERVAL_MS + 1),
               diagnostics::ValidationException);

  // Retry Count
  EXPECT_NO_THROW(InputValidator::validate_retry_count(0));                                     // Valid finite
  EXPECT_NO_THROW(InputValidator::validate_retry_count(base::constants::MAX_RETRIES_LIMIT));    // Valid finite max
  EXPECT_NO_THROW(InputValidator::validate_retry_count(base::constants::DEFAULT_MAX_RETRIES));  // -1, Valid infinite
  EXPECT_THROW(InputValidator::validate_retry_count(base::constants::MAX_RETRIES_LIMIT + 1),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_count(-2),
               diagnostics::ValidationException);  // Any value less than -1 or less than 0 but not -1
}

TEST(InputValidatorTest, ValidateGenericHelpers) {
  // Non-empty String
  EXPECT_NO_THROW(InputValidator::validate_non_empty_string("test", "name"));
  EXPECT_THROW(InputValidator::validate_non_empty_string("", "name"), diagnostics::ValidationException);

  // String Length
  EXPECT_NO_THROW(InputValidator::validate_string_length("test", 10, "string_field"));
  EXPECT_NO_THROW(InputValidator::validate_string_length("longstring", 10, "string_field"));
  EXPECT_THROW(InputValidator::validate_string_length("too long string", 10, "string_field"),
               diagnostics::ValidationException);

  // Positive Number
  EXPECT_NO_THROW(InputValidator::validate_positive_number(static_cast<int64_t>(1), "val"));
  EXPECT_THROW(InputValidator::validate_positive_number(static_cast<int64_t>(0), "val"),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_positive_number(static_cast<int64_t>(-1), "val"),
               diagnostics::ValidationException);

  // Validate Range (int64_t)
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<int64_t>(10), static_cast<int64_t>(0),
                                                 static_cast<int64_t>(20), "val"));
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<int64_t>(0), static_cast<int64_t>(0),
                                                 static_cast<int64_t>(20), "val"));
  EXPECT_NO_THROW(InputValidator::validate_range(static_cast<int64_t>(20), static_cast<int64_t>(0),
                                                 static_cast<int64_t>(20), "val"));
  EXPECT_THROW(InputValidator::validate_range(static_cast<int64_t>(-1), static_cast<int64_t>(0),
                                              static_cast<int64_t>(20), "val"),
               diagnostics::ValidationException);
  EXPECT_THROW(InputValidator::validate_range(static_cast<int64_t>(21), static_cast<int64_t>(0),
                                              static_cast<int64_t>(20), "val"),
               diagnostics::ValidationException);

  // Validate Range (size_t)
  EXPECT_NO_THROW(
      InputValidator::validate_range(static_cast<size_t>(10), static_cast<size_t>(0), static_cast<size_t>(20), "val"));
  EXPECT_NO_THROW(
      InputValidator::validate_range(static_cast<size_t>(0), static_cast<size_t>(0), static_cast<size_t>(20), "val"));
  EXPECT_NO_THROW(
      InputValidator::validate_range(static_cast<size_t>(20), static_cast<size_t>(0), static_cast<size_t>(20), "val"));
  EXPECT_THROW(
      InputValidator::validate_range(static_cast<size_t>(21), static_cast<size_t>(0), static_cast<size_t>(20), "val"),
      diagnostics::ValidationException);

  // Validate memory alignment
  // Note: validate_memory_alignment signature is (const void* ptr, size_t alignment);
  // Test valid aligned pointer
  void* aligned_ptr = reinterpret_cast<void*>(0x1000);  // Dummy pointer, assuming 0x1000 is 8-byte aligned
  EXPECT_NO_THROW(InputValidator::validate_memory_alignment(aligned_ptr, 8));
  // Test null pointer
  EXPECT_THROW(InputValidator::validate_memory_alignment(nullptr, 8), diagnostics::ValidationException);
  // Test unaligned pointer (requires specific setup or dynamically allocated unaligned memory)
  // For simplicity, we can simulate an unaligned pointer from a valid one.
  void* unaligned_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(aligned_ptr) + 1);  // Not 8-byte aligned
  EXPECT_THROW(InputValidator::validate_memory_alignment(unaligned_ptr, 8), diagnostics::ValidationException);
}

TEST(InputValidatorTest, DetailedHelperLogic) {
  // IPv4 Edge Cases
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.2.3"), diagnostics::ValidationException);  // Too few octets
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.2.3.4.5"),
               diagnostics::ValidationException);                                                   // Too many octets
  EXPECT_THROW(InputValidator::validate_ipv4_address("1..3.4"), diagnostics::ValidationException);  // Empty octet
  EXPECT_THROW(InputValidator::validate_ipv4_address(".1.2.3"), diagnostics::ValidationException);  // Empty first octet
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.2.3."), diagnostics::ValidationException);  // Empty last octet
  EXPECT_THROW(InputValidator::validate_ipv4_address("01.1.1.1"), diagnostics::ValidationException);  // Leading zero
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.01.1.1"), diagnostics::ValidationException);  // Leading zero
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.1.1.01"), diagnostics::ValidationException);  // Leading zero
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.a.1.1"), diagnostics::ValidationException);   // Non-digit
  EXPECT_THROW(InputValidator::validate_ipv4_address("1.1.1.256"),
               diagnostics::ValidationException);  // Out of range > 255
  EXPECT_THROW(InputValidator::validate_ipv4_address("256.1.1.1"),
               diagnostics::ValidationException);  // Out of range > 255

  // IPv6 Edge Cases (Regex based)
  EXPECT_THROW(InputValidator::validate_ipv6_address("1:2"), diagnostics::ValidationException);   // Malformed
  EXPECT_THROW(InputValidator::validate_ipv6_address("g::1"), diagnostics::ValidationException);  // Invalid hex char

  // Hostname Edge Cases
  EXPECT_THROW(InputValidator::validate_host("-test.com"), diagnostics::ValidationException);  // Starts with hyphen
  EXPECT_THROW(InputValidator::validate_host("test.com-"), diagnostics::ValidationException);  // Ends with hyphen
  EXPECT_THROW(InputValidator::validate_host("invalid_host.com"),
               diagnostics::ValidationException);                                              // Underscore invalid
  EXPECT_THROW(InputValidator::validate_host("test..com"), diagnostics::ValidationException);  // Empty label

  std::string long_label(64, 'a');
  EXPECT_THROW(InputValidator::validate_host(long_label + ".com"), diagnostics::ValidationException);  // Label too long

  // Device Path Edge Cases
  EXPECT_THROW(InputValidator::validate_device_path("/dev/bad?"),
               diagnostics::ValidationException);  // Invalid char in unix path
  EXPECT_THROW(InputValidator::validate_device_path("COM"), diagnostics::ValidationException);     // Incomplete COM
  EXPECT_THROW(InputValidator::validate_device_path("COM0"), diagnostics::ValidationException);    // COM0 invalid
  EXPECT_THROW(InputValidator::validate_device_path("COM256"), diagnostics::ValidationException);  // COM256 invalid
  EXPECT_THROW(InputValidator::validate_device_path("COM1a"),
               diagnostics::ValidationException);  // Invalid number format

  // Valid Windows Special Names (as per code)
  EXPECT_NO_THROW(InputValidator::validate_device_path("NUL"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("CON"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("PRN"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("AUX"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("LPT1"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("LPT2"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("LPT3"));
}