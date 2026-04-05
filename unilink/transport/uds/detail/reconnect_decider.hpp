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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>

#include "unilink/config/uds_config.hpp"
#include "unilink/diagnostics/error_types.hpp"
#include "unilink/transport/tcp_client/reconnect_policy.hpp"

namespace unilink {
namespace transport {
namespace detail {

// Keep the same cap for consistency.
constexpr auto MAX_RECONNECT_DELAY = std::chrono::milliseconds(30000);

/**
 * @brief Represents the decision on whether to retry a connection attempt for UDS.
 */
struct UdsReconnectLogicDecision {
  bool should_retry{false};
  std::optional<std::chrono::milliseconds> delay{std::nullopt};
};

inline std::chrono::milliseconds clamp_reconnect_delay_uds(std::chrono::milliseconds delay) {
  if (delay < std::chrono::milliseconds(0)) {
    return std::chrono::milliseconds(0);
  }
  return std::min(delay, MAX_RECONNECT_DELAY);
}

/**
 * @brief Determines whether a UDS reconnection attempt should be made.
 */
inline UdsReconnectLogicDecision decide_reconnect_uds(const config::UdsClientConfig& cfg,
                                                      const diagnostics::ErrorInfo& error_info, uint32_t attempt_count,
                                                      const std::optional<ReconnectPolicy>& policy) {
  if (!error_info.retryable) {
    return {false, std::nullopt};
  }

  if (cfg.max_retries == 0) {
    return {false, std::nullopt};
  }

  if (cfg.max_retries > 0 && attempt_count >= static_cast<uint32_t>(cfg.max_retries)) {
    return {false, std::nullopt};
  }

  if (policy) {
    auto policy_decision = (*policy)(error_info, attempt_count);
    if (!policy_decision.retry) {
      return {false, std::nullopt};
    }
    return {true, clamp_reconnect_delay_uds(policy_decision.delay)};
  }

  return {true, clamp_reconnect_delay_uds(std::chrono::milliseconds(cfg.retry_interval_ms))};
}

}  // namespace detail
}  // namespace transport
}  // namespace unilink
