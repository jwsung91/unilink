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

#pragma once

#include <string>

#include "unilink/common/constants.hpp"

namespace unilink {
namespace config {

struct SerialConfig {
#ifdef _WIN32
  std::string device = "COM1";
#else
  std::string device = "/dev/ttyUSB0";
#endif
  unsigned baud_rate = 115200;
  unsigned char_size = 8;  // 5,6,7,8
  enum class Parity { None, Even, Odd } parity = Parity::None;
  unsigned stop_bits = 1;  // 1 or 2
  enum class Flow { None, Software, Hardware } flow = Flow::None;

  size_t read_chunk = common::constants::DEFAULT_READ_BUFFER_SIZE;
  bool reopen_on_error = true;  // Attempt to reopen on device disconnection/error
  size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  bool enable_memory_pool = true;

  unsigned retry_interval_ms = common::constants::DEFAULT_RETRY_INTERVAL_MS;
  int max_retries = common::constants::DEFAULT_MAX_RETRIES;

  // Validation methods
  bool is_valid() const {
    return !device.empty() && baud_rate > 0 && char_size >= 5 && char_size <= 8 && (stop_bits == 1 || stop_bits == 2) &&
           retry_interval_ms >= common::constants::MIN_RETRY_INTERVAL_MS &&
           retry_interval_ms <= common::constants::MAX_RETRY_INTERVAL_MS &&
           backpressure_threshold >= common::constants::MIN_BACKPRESSURE_THRESHOLD &&
           backpressure_threshold <= common::constants::MAX_BACKPRESSURE_THRESHOLD &&
           (max_retries == -1 || (max_retries >= 0 && max_retries <= common::constants::MAX_RETRIES_LIMIT));
  }

  // Apply validation and clamp values to valid ranges
  void validate_and_clamp() {
    if (char_size < 5)
      char_size = 5;
    else if (char_size > 8)
      char_size = 8;

    if (stop_bits != 1 && stop_bits != 2) stop_bits = 1;

    if (retry_interval_ms < common::constants::MIN_RETRY_INTERVAL_MS) {
      retry_interval_ms = common::constants::MIN_RETRY_INTERVAL_MS;
    } else if (retry_interval_ms > common::constants::MAX_RETRY_INTERVAL_MS) {
      retry_interval_ms = common::constants::MAX_RETRY_INTERVAL_MS;
    }

    if (backpressure_threshold < common::constants::MIN_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;
    } else if (backpressure_threshold > common::constants::MAX_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MAX_BACKPRESSURE_THRESHOLD;
    }

    if (max_retries != -1 && max_retries > common::constants::MAX_RETRIES_LIMIT) {
      max_retries = common::constants::MAX_RETRIES_LIMIT;
    }
  }
};

}  // namespace config
}  // namespace unilink
