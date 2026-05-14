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

#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include "unilink/base/constants.hpp"

namespace unilink {
namespace transport {
namespace queue_util {

// Returns the byte size of a buffer held in a transport BufferVariant alternative.
// shared_ptr<const vector<uint8_t>> goes through ->size(); everything else via .size().
template <typename T>
inline size_t variant_buffer_size(const T& buf) {
  if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
    return buf ? buf->size() : 0;
  } else {
    return buf.size();
  }
}

// BestEffort queue-trimming shared by all stream transports (TCP client/server, UDS client/server).
// Must be called on the strand immediately before enqueueing a new buffer of `added` bytes.
//
// No-op for Reliable strategy.
// For BestEffort:
//   added >= bp_high  →  drop entire tx_ (full keep-latest replacement).
//   otherwise         →  pop oldest tx_ entries until queue_bytes + added <= bp_high.
template <typename Deque>
inline void maybe_flush_for_keep_latest(::unilink::base::constants::BackpressureStrategy bp_strategy, size_t added,
                                        size_t bp_high, Deque& tx, std::atomic<size_t>& queue_bytes,
                                        const std::atomic<bool>& backpressure_active) {
  if (bp_strategy != ::unilink::base::constants::BackpressureStrategy::BestEffort) return;

  if (added >= bp_high) {
    size_t removed_bytes = 0;
    for (const auto& buf : tx) {
      removed_bytes += std::visit([](const auto& b) { return variant_buffer_size(b); }, buf);
    }
    tx.clear();
    const size_t qb = queue_bytes.load(std::memory_order_relaxed);
    queue_bytes.store(qb > removed_bytes ? qb - removed_bytes : 0, std::memory_order_relaxed);
    return;
  }

  if (backpressure_active.load(std::memory_order_relaxed) ||
      queue_bytes.load(std::memory_order_relaxed) + added > bp_high) {
    while (!tx.empty()) {
      const size_t qb = queue_bytes.load(std::memory_order_relaxed);
      if (qb + added <= bp_high) break;
      const size_t oldest = std::visit([](const auto& b) { return variant_buffer_size(b); }, tx.front());
      queue_bytes.store(qb > oldest ? qb - oldest : 0, std::memory_order_relaxed);
      tx.pop_front();
    }
  }
}

}  // namespace queue_util
}  // namespace transport
}  // namespace unilink
