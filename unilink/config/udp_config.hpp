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
#include <optional>
#include <string>

#include "unilink/base/constants.hpp"

namespace unilink {
namespace config {

struct UdpConfig {
  std::string local_address = "0.0.0.0";
  uint16_t local_port = 0;
  std::optional<std::string> remote_address;
  std::optional<uint16_t> remote_port;
  size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  bool enable_memory_pool = true;
  bool stop_on_callback_exception = false;

  UdpConfig() = default;
  ~UdpConfig() = default;
  UdpConfig(const UdpConfig&) = default;
  UdpConfig& operator=(const UdpConfig&) = default;
  UdpConfig(UdpConfig&&) noexcept = default;
  UdpConfig& operator=(UdpConfig&&) noexcept = default;

  bool is_valid() const {
    if (backpressure_threshold < common::constants::MIN_BACKPRESSURE_THRESHOLD ||
        backpressure_threshold > common::constants::MAX_BACKPRESSURE_THRESHOLD) {
      return false;
    }
    if (remote_address.has_value() != remote_port.has_value()) return false;
    if (remote_port && *remote_port == 0) return false;
    return true;
  }

  void validate_and_clamp() {
    if (backpressure_threshold < common::constants::MIN_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;
    } else if (backpressure_threshold > common::constants::MAX_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MAX_BACKPRESSURE_THRESHOLD;
    }
  }
};

}  // namespace config
}  // namespace unilink
