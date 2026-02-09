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

#include "unilink/framer/line_framer.hpp"

#include <algorithm>
#include <iterator>
#include <string_view>

namespace unilink {
namespace framer {

LineFramer::LineFramer(std::string_view delimiter, bool include_delimiter, size_t max_length)
    : delimiter_(delimiter), include_delimiter_(include_delimiter), max_length_(max_length) {
  if (delimiter_.empty()) {
    delimiter_ = "\n";
  }
}

void LineFramer::push_bytes(memory::ConstByteSpan data) {
  if (data.empty()) return;

  size_t offset = 0;
  while (offset < data.size()) {
    // If buffer has data, check for split delimiter at the boundary first
    if (!buffer_.empty()) {
      // Append a small chunk to check for completion
      // We take enough bytes to potentially complete a delimiter starting in buffer
      size_t peek_len = std::min(data.size() - offset, delimiter_.length());

      buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(offset),
                     data.begin() + static_cast<std::ptrdiff_t>(offset + peek_len));
      offset += peek_len;

      auto it_delim = std::search(buffer_.begin(), buffer_.end(), delimiter_.begin(), delimiter_.end());
      if (it_delim != buffer_.end()) {
        // Found delimiter!
        size_t msg_total_len = static_cast<size_t>(std::distance(buffer_.begin(), it_delim)) + delimiter_.length();
        if (on_message_) {
          size_t extract_len = include_delimiter_ ? msg_total_len : (msg_total_len - delimiter_.length());
          on_message_(memory::ConstByteSpan(buffer_.data(), extract_len));
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(msg_total_len));
        continue;
      }

      // Not found. Check overflow.
      if (buffer_.size() > max_length_) {
        buffer_.clear();
        // We consumed peek_len.
        // Continue loop to process remaining data
        continue;
      }
    }

    // Now search in rest of data
    std::string_view data_view(reinterpret_cast<const char*>(data.data()), data.size());
    size_t pos = data_view.find(delimiter_, offset);

    if (pos != std::string_view::npos) {
      // Found a delimiter at pos
      size_t chunk_len = pos - offset + delimiter_.length();

      // Check if appending this chunk would exceed max_length
      if (buffer_.size() + chunk_len > max_length_) {
        // Overflow - message too long
        buffer_.clear();
        offset += chunk_len;
        continue;
      }

      // Append the chunk (up to and including delimiter)
      buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(offset),
                     data.begin() + static_cast<std::ptrdiff_t>(offset + chunk_len));

      // Trigger callback
      if (on_message_) {
        size_t msg_len = buffer_.size() - (include_delimiter_ ? 0 : delimiter_.length());
        on_message_(memory::ConstByteSpan(buffer_.data(), msg_len));
      }

      // Clear buffer as we processed the message
      buffer_.clear();
      offset += chunk_len;

    } else {
      // No delimiter found in the rest of data
      size_t remaining = data.size() - offset;

      if (buffer_.size() + remaining > max_length_) {
        // Overflow - partial message already too long
        buffer_.clear();
        // Discard remaining data
        return;
      }

      // Append remaining data
      buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(offset), data.end());

      // Check for split delimiter formed at the boundary (again, for the final chunk)
      // Only need to search if buffer has enough data
      if (buffer_.size() >= delimiter_.length()) {
        auto it_delim = std::search(buffer_.begin(), buffer_.end(), delimiter_.begin(), delimiter_.end());
        if (it_delim != buffer_.end()) {
          // Found split delimiter!
          size_t msg_total_len = static_cast<size_t>(std::distance(buffer_.begin(), it_delim)) + delimiter_.length();

          if (on_message_) {
            size_t extract_len = include_delimiter_ ? msg_total_len : (msg_total_len - delimiter_.length());
            on_message_(memory::ConstByteSpan(buffer_.data(), extract_len));
          }

          // Remove processed part
          if (msg_total_len >= buffer_.size()) {
            buffer_.clear();
          } else {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(msg_total_len));
          }
        }
      }

      return;  // Done with data
    }
  }
}

void LineFramer::set_on_message(MessageCallback cb) { on_message_ = std::move(cb); }

void LineFramer::reset() { buffer_.clear(); }

}  // namespace framer
}  // namespace unilink
