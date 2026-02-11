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

#include <string>
#include <string_view>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/framer/iframer.hpp"

namespace unilink {
namespace framer {

/**
 * @brief Framer for text-based protocols (e.g., ASCII, NMEA).
 *
 * Buffers incoming data and extracts messages delimited by a specific sequence (e.g., "\n").
 */
class UNILINK_API LineFramer : public IFramer {
 public:
  /**
   * @brief Construct a new Line Framer
   *
   * @param delimiter The delimiter string (default: "\n")
   * @param include_delimiter Whether to include the delimiter in the extracted message (default: false)
   * @param max_length Maximum message length before forcing a reset (default: 65536)
   */
  explicit LineFramer(std::string_view delimiter = "\n", bool include_delimiter = false, size_t max_length = 65536);

  ~LineFramer() override = default;

  void push_bytes(memory::ConstByteSpan data) override;
  void set_on_message(MessageCallback cb) override;
  void reset() override;

 private:
  std::string delimiter_;
  bool include_delimiter_;
  size_t max_length_;

  size_t scanned_index_ = 0;
  std::vector<uint8_t> buffer_;
  MessageCallback on_message_;
};

}  // namespace framer
}  // namespace unilink
