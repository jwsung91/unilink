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

#include "unilink/base/constants.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/diagnostics/error_types.hpp"
#include "unilink/transport/tcp_client/reconnect_policy.hpp"

namespace unilink {
namespace transport {
namespace detail {

// Maximum allowed delay for reconnection attempts (30 seconds)
constexpr auto MAX_RECONNECT_DELAY = std::chrono::milliseconds(30000);

/**
 * @brief Represents the decision on whether to retry a connection attempt.
 */
struct ReconnectLogicDecision {
  bool should_retry{false};
  std::optional<std::chrono::milliseconds> delay{std::nullopt};
};

/**
 * @brief Determines whether a reconnection attempt should be made based on configuration and error state.
 *
 * @param cfg The client configuration.
 * @param error_info The error information from the last failure.
 * @param attempt_count The current attempt count (0-based).
 * @param policy The custom reconnection policy, if any.
 * @return ReconnectLogicDecision containing whether to retry and the optional delay.
 */
inline ReconnectLogicDecision decide_reconnect(const config::TcpClientConfig& cfg,
                                               const diagnostics::ErrorInfo& error_info, uint32_t attempt_count,
                                               const std::optional<ReconnectPolicy>& policy) {
  // If the error is not retryable, stop immediately.
  if (!error_info.retryable) {
    return {false, std::nullopt};
  }

  // If max_retries is 0, stop immediately.
  if (cfg.max_retries == 0) {
    return {false, std::nullopt};
  }

  // If max_retries is finite (> 0) and we've reached or exceeded it, stop.
  if (cfg.max_retries > 0 && attempt_count >= static_cast<uint32_t>(cfg.max_retries)) {
    return {false, std::nullopt};
  }

  // If a custom policy is provided, use it.
  if (policy) {
    auto decision = (*policy)(error_info, attempt_count);

    if (!decision.retry) {
      return {false, std::nullopt};
    }

    // Clamp delay to be non-negative and within reasonable bounds.
    auto delay_ms = decision.delay;
    if (delay_ms < std::chrono::milliseconds(0)) {
      delay_ms = std::chrono::milliseconds(0);
    }
    if (delay_ms > MAX_RECONNECT_DELAY) {
      delay_ms = MAX_RECONNECT_DELAY;
    }

    return {true, delay_ms};
  }

  // No custom policy, proceed with legacy logic (delay determined by caller).
  return {true, std::nullopt};
}

}  // namespace detail
}  // namespace transport
}  // namespace unilink
