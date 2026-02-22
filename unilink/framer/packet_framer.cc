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
#include <stdexcept>

namespace unilink {
namespace framer {

PacketFramer::PacketFramer(const std::vector<uint8_t>& start_pattern, const std::vector<uint8_t>& end_pattern,
                           size_t max_length)
    : start_pattern_(start_pattern), end_pattern_(end_pattern), max_length_(max_length), state_(State::Sync) {
  if (start_pattern_.empty() && end_pattern_.empty()) {
    throw std::invalid_argument("PacketFramer: start_pattern and end_pattern cannot both be empty.");
  }
}

void PacketFramer::push_bytes(memory::ConstByteSpan data) {
  if (data.empty()) return;

  // Fast path: if buffer is empty and we are looking for start pattern (and have one)
  // we can process data directly without copying to buffer first.
  if (buffer_.empty() && state_ == State::Sync && !start_pattern_.empty()) {
    size_t processed_idx = 0;

    while (processed_idx < data.size()) {
      auto search_start = data.begin() + static_cast<std::ptrdiff_t>(processed_idx);
      auto it_start = std::search(search_start, data.end(), start_pattern_.begin(), start_pattern_.end());

      if (it_start == data.end()) {
        // Start pattern not found.
        // Keep partial match at the end if any.
        if (start_pattern_.size() > 1) {
          size_t remaining = data.size() - processed_idx;
          size_t keep_len = start_pattern_.size() - 1;
          if (remaining > keep_len) {
            // We can skip the beginning
            size_t skip = remaining - keep_len;
            processed_idx += skip;
          }
          buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(processed_idx), data.end());
        }
        return;
      }

      // Found start pattern.
      size_t start_idx = static_cast<size_t>(std::distance(data.begin(), it_start));

      if (end_pattern_.empty()) {
        // Minimal packet is just start pattern
        size_t packet_len = start_pattern_.size();
        if (on_message_) {
          on_message_(memory::ConstByteSpan(data.data() + start_idx, packet_len));
        }
        processed_idx = start_idx + packet_len;
        // Continue loop
        continue;
      }

      // Search for end pattern *after* start pattern
      auto search_end_start = it_start + static_cast<std::ptrdiff_t>(start_pattern_.size());
      auto it_end = std::search(search_end_start, data.end(), end_pattern_.begin(), end_pattern_.end());

      if (it_end == data.end()) {
        // End pattern not found in this chunk.
        // We must buffer the partial packet (from start_idx to end)
        buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(start_idx), data.end());
        state_ = State::Collect;
        scanned_idx_ = buffer_.size();  // All scanned

        // Check max length immediately
        if (buffer_.size() > max_length_) {
          buffer_.clear();
          state_ = State::Sync;
          scanned_idx_ = 0;
        }
        return;
      }

      // Found end pattern.
      size_t end_idx = static_cast<size_t>(std::distance(data.begin(), it_end));
      size_t packet_len = (end_idx - start_idx) + end_pattern_.size();

      if (packet_len <= max_length_) {
        if (on_message_) {
          on_message_(memory::ConstByteSpan(data.data() + start_idx, packet_len));
        }
      } else {
        // Packet too long, discard.
      }

      // Advance processed_idx to end of packet
      processed_idx = start_idx + packet_len;
    }
    return;
  }

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
        // Start scanning for end pattern after the start pattern we just found
        scanned_idx_ = start_pattern_.size();
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
      // Optimization: use scanned_idx_ to avoid re-scanning
      size_t search_offset = std::max(start_pattern_.size(), scanned_idx_);

      // Back up slightly to catch split end pattern if we are resuming search
      if (search_offset > start_pattern_.size()) {
        size_t overlap = (end_pattern_.size() > 1) ? (end_pattern_.size() - 1) : 0;
        if (search_offset >= overlap) {
          search_offset -= overlap;
        } else {
          search_offset = 0;
        }
      }

      // Safety clamp to ensure we don't search inside start pattern
      if (search_offset < start_pattern_.size()) {
        search_offset = start_pattern_.size();
      }

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
          scanned_idx_ = 0;
        } else {
          // Exceeded max length, discard packet
          buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(packet_len));
          state_ = State::Sync;
          scanned_idx_ = 0;
        }
      } else {
        // End pattern not found.
        scanned_idx_ = buffer_.size();
        if (buffer_.size() > max_length_) {
          // Exceeded limit while collecting. Reset.
          buffer_.clear();
          state_ = State::Sync;
          scanned_idx_ = 0;
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
  scanned_idx_ = 0;
}

}  // namespace framer
}  // namespace unilink
