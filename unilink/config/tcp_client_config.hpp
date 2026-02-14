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
#include "unilink/util/input_validator.hpp"

namespace unilink {
namespace config {

struct TcpClientConfig {
  std::string host = "127.0.0.1";
  uint16_t port = 9000;
  unsigned retry_interval_ms = common::constants::DEFAULT_RETRY_INTERVAL_MS;
  unsigned connection_timeout_ms = common::constants::DEFAULT_CONNECTION_TIMEOUT_MS;
  int max_retries = common::constants::DEFAULT_MAX_RETRIES;
  size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  bool enable_memory_pool = true;

  TcpClientConfig() = default;
  TcpClientConfig(const TcpClientConfig&) = default;
  TcpClientConfig& operator=(const TcpClientConfig&) = default;
  TcpClientConfig(TcpClientConfig&&) noexcept = default;
  TcpClientConfig& operator=(TcpClientConfig&&) noexcept = default;

  // Validation methods
  bool is_valid() const {
    return util::InputValidator::is_valid_host(host) && port > 0 &&
           retry_interval_ms >= common::constants::MIN_RETRY_INTERVAL_MS &&
           retry_interval_ms <= common::constants::MAX_RETRY_INTERVAL_MS &&
           backpressure_threshold >= common::constants::MIN_BACKPRESSURE_THRESHOLD &&
           backpressure_threshold <= common::constants::MAX_BACKPRESSURE_THRESHOLD &&
           (max_retries == -1 || (max_retries >= 0 && max_retries <= common::constants::MAX_RETRIES_LIMIT));
  }

  // Apply validation and clamp values to valid ranges
  void validate_and_clamp() {
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
