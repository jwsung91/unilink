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

  size_t data_offset = 0;

  // Fast path: if buffer is empty, process data directly to avoid copy and allocation
  if (buffer_.empty()) {
    std::string_view view(reinterpret_cast<const char*>(data.data()), data.size());
    size_t current_pos = 0;

    while (true) {
      size_t pos = view.find(delimiter_, current_pos);
      if (pos == std::string_view::npos) {
        // No more delimiters in data.
        data_offset = current_pos;
        break;
      }

      // Found a delimiter at pos
      size_t msg_len = pos - current_pos;
      size_t total_len = msg_len + delimiter_.length();

      // Check for overflow
      if (total_len > max_length_) {
        // Message too long. Reset state (effectively no-op for empty buffer, but we skip this message)
        // Since buffer is empty, we just skip this message and any potential partial data?
        // Usually reset clears buffer. Here we just consume the data and continue?
        // Or stop processing?
        // Consistent behavior: clear buffer and stop.
        // Since buffer is empty, we just return.
        // But we should consume the data?
        // If we reset, we lose the stream context.
        return;
      }

      // Emit message
      if (on_message_) {
        size_t payload_len = include_delimiter_ ? total_len : msg_len;
        on_message_(memory::ConstByteSpan(data.data() + current_pos, payload_len));
      }

      current_pos += total_len;
    }
  }

  // If we consumed all data in fast path
  if (data_offset >= data.size()) {
    return;
  }

  // Buffered path: append remaining data
  // Note: We append all remaining data to avoid complex chunking logic.
  // This assumes data chunks are reasonably sized or max_length_ checks will catch overflows eventually.
  buffer_.insert(buffer_.end(), data.begin() + static_cast<std::ptrdiff_t>(data_offset), data.end());

  // Prepare for search in buffer
  // We start searching from scanned_index_ minus overlap to handle split delimiters
  size_t search_start = scanned_index_;
  if (search_start >= delimiter_.length()) {
    search_start -= (delimiter_.length() - 1);
  } else {
    search_start = 0;
  }

  // Ensure search_start is valid
  if (search_start > buffer_.size()) {
    search_start = buffer_.size();
  }

  auto search_it = buffer_.begin() + static_cast<std::ptrdiff_t>(search_start);
  size_t processed_count = 0;

  while (true) {
    auto it = std::search(search_it, buffer_.end(), delimiter_.begin(), delimiter_.end());

    if (it == buffer_.end()) {
      // No delimiter found
      break;
    } else {
      // Found delimiter
      size_t msg_end_index = static_cast<size_t>(std::distance(buffer_.begin(), it));
      // Length of this specific message (from processed_count to delimiter end)
      size_t current_msg_total_len = (msg_end_index - processed_count) + delimiter_.length();

      // Check max_length_
      if (current_msg_total_len > max_length_) {
        // Message too long. Reset.
        buffer_.clear();
        scanned_index_ = 0;
        return;
      }

      if (on_message_) {
        size_t payload_len = include_delimiter_ ? current_msg_total_len : (current_msg_total_len - delimiter_.length());
        on_message_(memory::ConstByteSpan(buffer_.data() + processed_count, payload_len));
      }

      // Advance processed_count
      processed_count += current_msg_total_len;

      // Advance search_it (strictly after the current message)
      search_it = buffer_.begin() + static_cast<std::ptrdiff_t>(processed_count);
    }
  }

  // Remove processed messages in one go (delayed erase)
  if (processed_count > 0) {
    buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(processed_count));
  }

  // Update scanned_index_
  // We searched up to the end of the buffer (which is now smaller)
  scanned_index_ = buffer_.size();

  // Check max_length_ for the remaining partial message
  if (buffer_.size() > max_length_) {
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
