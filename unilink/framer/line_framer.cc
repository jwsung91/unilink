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

  // Process data in chunks to prevent large memory allocations (DoS protection)
  // We use max(max_length_, 4096) as a reasonable chunk size.
  // This ensures that even if the user sends a huge payload, we only
  // allocate memory incrementally and have a chance to clear the buffer
  // if limits are exceeded.
  const size_t chunk_limit = std::max(max_length_, size_t(4096));

  size_t offset = 0;
  while (offset < data.size()) {
    size_t len = std::min(data.size() - offset, chunk_limit);
    push_bytes_internal(data.subspan(offset, len));
    offset += len;
  }
}

void LineFramer::push_bytes_internal(memory::ConstByteSpan data) {
  if (data.empty()) return;

  // Fast Path: If buffer is empty, process data directly (zero-copy)
  if (buffer_.empty()) {
    size_t processed_count = scan_and_process(data, 0);

    // If we haven't processed everything, append the remainder to the buffer
    if (processed_count < data.size()) {
      buffer_.insert(buffer_.end(), data.begin() + processed_count, data.end());
      scanned_idx_ = buffer_.size();  // We scanned all of it

      // DoS protection for partial message overflow
      if (buffer_.size() > max_length_) {
        buffer_.clear();
        scanned_idx_ = 0;
      }
    } else {
      // All processed, buffer remains empty
      scanned_idx_ = 0;
    }
    return;
  }

  // Slow Path: Append new data to buffer and process
  buffer_.insert(buffer_.end(), data.begin(), data.end());

  // Determine where to start searching to avoid re-scanning
  // We back up by delimiter length - 1 to catch split delimiters
  size_t search_start_idx = scanned_idx_;
  if (search_start_idx >= delimiter_.length()) {
    search_start_idx -= (delimiter_.length() - 1);
  } else {
    search_start_idx = 0;
  }

  size_t processed_count = scan_and_process(memory::ConstByteSpan(buffer_), search_start_idx);

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

size_t LineFramer::scan_and_process(memory::ConstByteSpan data, size_t search_start_offset) {
  // Safety clamp
  if (search_start_offset > data.size()) {
    search_start_offset = data.size();
  }

  // processed_count tracks the number of bytes from the start of the buffer
  // that have been either emitted as messages or skipped due to overflow.
  size_t processed_count = 0;

  // Search cursor
  size_t search_cursor = search_start_offset;

  // O(N) scan loop
  while (true) {
    if (search_cursor > data.size()) break;

    // Perform search using iterators derived from current buffer state
    auto search_begin = data.begin() + static_cast<std::ptrdiff_t>(search_cursor);
    decltype(data.begin()) it;

    if (delimiter_.size() == 1) {
      // Optimization: Use std::find for single-byte delimiter
      it = std::find(search_begin, data.end(), static_cast<uint8_t>(delimiter_[0]));
    } else {
      it = std::search(search_begin, data.end(), delimiter_.begin(), delimiter_.end());
    }

    if (it == data.end()) {
      break;
    }

    // Found a delimiter
    size_t match_start_idx = static_cast<size_t>(std::distance(data.begin(), it));
    size_t match_end_idx = match_start_idx + delimiter_.length();

    // Calculate message length (from end of previous processed data to end of current delimiter)
    size_t current_msg_total_len = match_end_idx - processed_count;

    // Check strict max_length constraint
    if (current_msg_total_len > max_length_) {
      // Message exceeds limit. Skip it.
    } else {
      // Valid message
      if (on_message_) {
        size_t payload_len = include_delimiter_ ? current_msg_total_len : (current_msg_total_len - delimiter_.length());
        on_message_(memory::ConstByteSpan(data.data() + processed_count, payload_len));
      }
    }

    // Mark these bytes as processed
    processed_count = match_end_idx;

    // Advance search cursor to start strictly after the current delimiter
    search_cursor = processed_count;
  }

  return processed_count;
}

void LineFramer::set_on_message(MessageCallback cb) { on_message_ = std::move(cb); }

void LineFramer::reset() {
  buffer_.clear();
  scanned_idx_ = 0;
}

}  // namespace framer
}  // namespace unilink
