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

#include "unilink/framer/packet_framer.hpp"

#include <algorithm>
#include <iterator>

namespace unilink {
namespace framer {

PacketFramer::PacketFramer(const std::vector<uint8_t>& start_pattern, const std::vector<uint8_t>& end_pattern,
                           size_t max_length)
    : start_pattern_(start_pattern), end_pattern_(end_pattern), max_length_(max_length), state_(State::Sync) {}

void PacketFramer::push_bytes(memory::ConstByteSpan data) {
  if (data.empty()) return;

  buffer_.insert(buffer_.end(), data.begin(), data.end());

  while (true) {
    if (state_ == State::Sync) {
      if (start_pattern_.empty()) {
        state_ = State::Collect;
        continue;
      }

      auto it = std::search(buffer_.begin(), buffer_.end(), start_pattern_.begin(), start_pattern_.end());
      if (it != buffer_.end()) {
        // Found start pattern.
        // Discard everything before start pattern.
        if (it != buffer_.begin()) {
          buffer_.erase(buffer_.begin(), it);
        }
        state_ = State::Collect;
        // Continue to check for end pattern immediately
      } else {
        // Start pattern not found.
        // Keep partial match at the end.
        if (start_pattern_.size() > 1) {
          size_t keep_len = start_pattern_.size() - 1;
          if (buffer_.size() > keep_len) {
            buffer_.erase(buffer_.begin(), buffer_.end() - static_cast<std::ptrdiff_t>(keep_len));
          }
        } else {
          buffer_.clear();
        }
        break;  // Need more data
      }
    } else if (state_ == State::Collect) {
      if (end_pattern_.empty()) {
        // If end pattern is empty, packet ends immediately after start pattern?
        // Assume minimal packet is start pattern only
        size_t packet_len = start_pattern_.size();
        if (on_message_) {
          on_message_(memory::ConstByteSpan(buffer_.data(), packet_len));
        }
        if (buffer_.empty()) return;

        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(packet_len));
        state_ = State::Sync;
        continue;
      }

      // Search for end pattern *after* start pattern
      size_t search_offset = start_pattern_.size();
      if (buffer_.size() < search_offset) {
        // Should not happen if Sync worked correctly
        break;
      }

      auto search_start = buffer_.begin() + static_cast<std::ptrdiff_t>(search_offset);
      auto it = std::search(search_start, buffer_.end(), end_pattern_.begin(), end_pattern_.end());

      if (it != buffer_.end()) {
        // Found end pattern.
        size_t packet_len = static_cast<size_t>(std::distance(buffer_.begin(), it)) + end_pattern_.size();

        if (packet_len <= max_length_) {
          if (on_message_) {
            on_message_(memory::ConstByteSpan(buffer_.data(), packet_len));
          }
          if (buffer_.empty()) return;

          buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(packet_len));
          state_ = State::Sync;
        } else {
          // Exceeded max length, discard packet
          buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(packet_len));
          state_ = State::Sync;
        }
      } else {
        // End pattern not found.
        if (buffer_.size() > max_length_) {
          // Exceeded limit while collecting. Reset.
          buffer_.clear();
          state_ = State::Sync;
        }
        break;  // Need more data
      }
    }
  }
}

void PacketFramer::set_on_message(MessageCallback cb) { on_message_ = std::move(cb); }

void PacketFramer::reset() {
  buffer_.clear();
  state_ = State::Sync;
}

}  // namespace framer
}  // namespace unilink
