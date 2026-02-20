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

  // Optimization: Fast path for zero-copy processing when buffer is empty
  // This avoids copying data to the internal buffer if the input contains complete messages.
  if (buffer_.empty()) {
    size_t processed_count = 0;
    size_t search_cursor = 0;

    while (true) {
      if (search_cursor >= data.size()) break;

      auto search_begin = data.begin() + static_cast<std::ptrdiff_t>(search_cursor);
      decltype(data.begin()) it;

      if (delimiter_.size() == 1) {
        it = std::find(search_begin, data.end(), static_cast<uint8_t>(delimiter_[0]));
      } else {
        it = std::search(search_begin, data.end(), delimiter_.begin(), delimiter_.end());
      }

      if (it == data.end()) {
        // No more delimiters found.
        size_t remaining_len = data.size() - processed_count;
        if (remaining_len > max_length_) {
          // The partial message exceeds max length. Discard it.
          // Buffer remains empty, scanned_idx_ is 0.
          return;
        }

        // Append remaining partial message to buffer
        buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(processed_count), data.end());
        // We have scanned everything in the buffer
        scanned_idx_ = buffer_.size();
        return;
      }

      // Found a delimiter
      size_t match_start_idx = static_cast<size_t>(std::distance(data.begin(), it));
      size_t match_end_idx = match_start_idx + delimiter_.length();
      size_t current_msg_total_len = match_end_idx - processed_count;

      if (current_msg_total_len <= max_length_) {
        // Valid message - Emit directly from input data (Zero Copy)
        if (on_message_) {
          size_t payload_len =
              include_delimiter_ ? current_msg_total_len : (current_msg_total_len - delimiter_.length());
          on_message_(data.subspan(processed_count, payload_len));
        }
      } else {
        // Message exceeds limit. Skip it.
      }

      // Mark these bytes as processed
      processed_count = match_end_idx;
      search_cursor = processed_count;
    }
    // All data processed
    return;
  }

  // Append new data to buffer
  // We utilize a single buffered path to ensure O(N) complexity via the scanned_idx_ optimization.
  buffer_.insert(buffer_.end(), data.begin(), data.end());

  // Determine where to start searching to avoid re-scanning
  // We back up by delimiter length - 1 to catch split delimiters
  size_t search_start_idx = scanned_idx_;
  if (search_start_idx >= delimiter_.length()) {
    search_start_idx -= (delimiter_.length() - 1);
  } else {
    search_start_idx = 0;
  }

  // Safety clamp
  if (search_start_idx > buffer_.size()) {
    search_start_idx = buffer_.size();
  }

  // processed_count tracks the number of bytes from the start of the buffer
  // that have been either emitted as messages or skipped due to overflow.
  // These bytes will be erased at the end.
  size_t processed_count = 0;

  // Search cursor
  size_t search_cursor = search_start_idx;

  // O(N) scan loop: resumes from search_start_idx
  while (true) {
    if (search_cursor > buffer_.size()) break;

    // Perform search using iterators derived from current buffer state
    auto search_begin = buffer_.begin() + static_cast<std::ptrdiff_t>(search_cursor);
    decltype(buffer_.begin()) it;

    if (delimiter_.size() == 1) {
      // Optimization: Use std::find for single-byte delimiter (often maps to memchr)
      it = std::find(search_begin, buffer_.end(), static_cast<uint8_t>(delimiter_[0]));
    } else {
      it = std::search(search_begin, buffer_.end(), delimiter_.begin(), delimiter_.end());
    }

    if (it == buffer_.end()) {
      // No more delimiters found.
      // Update scanned_idx_ to the current buffer size, so next time we start searching from here.
      // Note: We effectively scanned everything remaining in the buffer.
      break;
    }

    // Found a delimiter
    size_t match_start_idx = static_cast<size_t>(std::distance(buffer_.begin(), it));
    size_t match_end_idx = match_start_idx + delimiter_.length();

    // Calculate message length (from end of previous processed data to end of current delimiter)
    size_t current_msg_total_len = match_end_idx - processed_count;

    // Check strict max_length constraint
    if (current_msg_total_len > max_length_) {
      // Message exceeds limit. Skip it.
      // We do NOT emit this message.
      // We essentially "consume" it by advancing processed_count.
    } else {
      // Valid message
      if (on_message_) {
        size_t payload_len = include_delimiter_ ? current_msg_total_len : (current_msg_total_len - delimiter_.length());
        on_message_(memory::ConstByteSpan(buffer_.data() + processed_count, payload_len));
      }
    }

    // Mark these bytes as processed
    processed_count = match_end_idx;

    // Advance search cursor to start strictly after the current delimiter
    search_cursor = processed_count;
  }

  // Batch erase all processed data to ensure O(N) erase complexity
  if (processed_count > 0) {
    buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(processed_count));
  }

  // Update scanned_idx_ for the next call.
  // The buffer size is now reduced. We have scanned everything that remains.
  scanned_idx_ = buffer_.size();

  // Final check: if the *remaining* partial message in the buffer already exceeds max_length_,
  // we must reset to prevent unbound growth (DoS protection).
  if (buffer_.size() > max_length_) {
    buffer_.clear();
    scanned_idx_ = 0;
  }
}

void LineFramer::set_on_message(MessageCallback cb) { on_message_ = std::move(cb); }

void LineFramer::reset() {
  buffer_.clear();
  scanned_idx_ = 0;
}

}  // namespace framer
}  // namespace unilink
