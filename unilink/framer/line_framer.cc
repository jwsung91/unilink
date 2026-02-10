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
    : delimiter_(delimiter), include_delimiter_(include_delimiter), max_length_(max_length), scanned_index_(0) {
  if (delimiter_.empty()) {
    delimiter_ = "\n";
  }
}

void LineFramer::push_bytes(memory::ConstByteSpan data) {
  if (data.empty()) return;

  // Append new data to buffer
  buffer_.insert(buffer_.end(), data.begin(), data.end());

  // Determine where to start searching.
  // We need to backtrack by (delimiter_.length() - 1) to catch split delimiters.
  // But we must not go below 0.
  // scanned_index_ points to where we finished scanning last time (or 0).
  size_t search_start = scanned_index_;
  if (search_start >= delimiter_.length() - 1) {
    search_start -= (delimiter_.length() - 1);
  } else {
    search_start = 0;
  }

  // To avoid erasing from the beginning of the vector repeatedly (which is O(N) per erase),
  // we will process all messages in the buffer first, and erase only once at the end.
  // We track the end of the last processed message.
  size_t last_processed_end = 0;

  // Search range starts from search_start
  auto search_begin = buffer_.begin() + static_cast<std::ptrdiff_t>(search_start);

  while (true) {
    auto it = std::search(search_begin, buffer_.end(), delimiter_.begin(), delimiter_.end());

    if (it == buffer_.end()) {
      // No more delimiters found.
      // Update scanned_index_ to the end of the buffer, so next time we start searching from here.
      scanned_index_ = buffer_.size();
      break;
    }

    // Found a delimiter.
    size_t found_pos = static_cast<size_t>(std::distance(buffer_.begin(), it));
    size_t msg_end = found_pos + delimiter_.length();

    // Calculate message length (including delimiter)
    // The message starts at 'last_processed_end' (start of unprocessed buffer)
    // Wait, last_processed_end is relative to original buffer_.begin().
    size_t msg_total_len = msg_end - last_processed_end;

    // Check individual message length constraint
    if (msg_total_len > max_length_) {
      // Message too long. We must reset/skip.
      // We effectively drop this message by advancing last_processed_end.
      last_processed_end = msg_end;
    } else {
      // Valid message. Emit it.
      if (on_message_) {
        size_t extract_len = include_delimiter_ ? msg_total_len : (msg_total_len - delimiter_.length());
        on_message_(memory::ConstByteSpan(buffer_.data() + last_processed_end, extract_len));
      }
      last_processed_end = msg_end;
    }

    // Advance search to start after this message
    search_begin = buffer_.begin() + static_cast<std::ptrdiff_t>(last_processed_end);
  }

  // Remove processed messages from the buffer
  if (last_processed_end > 0) {
    buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(last_processed_end));
    // Reset scanned_index_ to reflect the new buffer size (since we scanned everything)
    scanned_index_ = buffer_.size();
  }

  // Check if remaining partial buffer exceeds max_length
  if (buffer_.size() > max_length_) {
    // If we have accumulated too much without finding a delimiter, reset.
    buffer_.clear();
    scanned_index_ = 0;
  }
}

void LineFramer::set_on_message(MessageCallback cb) { on_message_ = std::move(cb); }

void LineFramer::reset() {
  buffer_.clear();
  scanned_index_ = 0;
}

}  // namespace framer
}  // namespace unilink
