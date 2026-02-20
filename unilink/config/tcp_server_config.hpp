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

#include <cstdint>
#include <string>

#include "unilink/base/constants.hpp"

namespace unilink {
namespace config {

struct TcpServerConfig {
  uint16_t port = 9000;
  std::string bind_address = "0.0.0.0";
  size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  bool enable_memory_pool = true;
  int max_connections = 100;  // Maximum concurrent connections

  // Port binding retry configuration
  bool enable_port_retry = false;     // Enable port binding retry
  int max_port_retries = 3;           // Maximum number of retry attempts
  int port_retry_interval_ms = 1000;  // Retry interval in milliseconds

  int idle_timeout_ms = 0;  // Idle connection timeout in milliseconds (0 = disabled)

  // Validation methods
  bool is_valid() const {
    return port > 0 && !bind_address.empty() &&
           backpressure_threshold >= common::constants::MIN_BACKPRESSURE_THRESHOLD &&
           backpressure_threshold <= common::constants::MAX_BACKPRESSURE_THRESHOLD && max_connections > 0 &&
           idle_timeout_ms >= 0;
  }

  // Apply validation and clamp values to valid ranges
  void validate_and_clamp() {
    if (backpressure_threshold < common::constants::MIN_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;
    } else if (backpressure_threshold > common::constants::MAX_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MAX_BACKPRESSURE_THRESHOLD;
    }

    if (max_connections <= 0) {
      max_connections = 1;
    }

    if (idle_timeout_ms < 0) {
      idle_timeout_ms = 0;
    }
  }
};

}  // namespace config
}  // namespace unilink
