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

  // Append new data to buffer
  buffer_.insert(buffer_.end(), data.begin(), data.end());

  size_t processed_offset = 0;

  while (processed_offset < buffer_.size()) {
    auto it_start = buffer_.begin() + static_cast<std::ptrdiff_t>(processed_offset);
    auto delimiter_pos = std::search(it_start, buffer_.end(), delimiter_.begin(), delimiter_.end());

    if (delimiter_pos == buffer_.end()) {
      break;
    }

    size_t msg_len = static_cast<size_t>(std::distance(it_start, delimiter_pos));
    size_t total_len = msg_len + delimiter_.length();

    if (on_message_) {
      size_t extract_len = include_delimiter_ ? total_len : msg_len;
      if (extract_len > 0) {
        // Address of element is safe as long as buffer is not reallocated
        on_message_(memory::ConstByteSpan(&(*it_start), extract_len));
      } else {
        // Empty message (only if msg_len=0 and !include_delimiter_)
        on_message_(memory::ConstByteSpan());
      }
    }

    // Check if reset was called during callback (buffer cleared)
    if (buffer_.empty()) {
      return;
    }

    processed_offset += total_len;
  }

  // Remove processed data
  if (processed_offset > 0) {
    if (processed_offset >= buffer_.size()) {
      buffer_.clear();
    } else {
      buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(processed_offset));
    }
  }

  // Check max length on remaining buffer
  if (buffer_.size() > max_length_) {
    // Buffer overflow - clear buffer to prevent attack/leak
    buffer_.clear();
  }
}

void LineFramer::set_on_message(MessageCallback cb) {
  on_message_ = std::move(cb);
}

void LineFramer::reset() {
  buffer_.clear();
}

}  // namespace framer
}  // namespace unilink
