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
#include <functional>
#include <memory>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/base/visibility.hpp"

namespace unilink {
namespace interface {
class UNILINK_API Channel {
 public:
  using OnBytes = std::function<void(const uint8_t*, size_t)>;
  using OnState = std::function<void(common::LinkState)>;
  using OnBackpressure = std::function<void(size_t /*queued_bytes*/)>;

  virtual ~Channel() = default;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool is_connected() const = 0;

  // Single send API (copies into internal queue)
  virtual void async_write_copy(const uint8_t* data, size_t size) = 0;
  // Zero-copy APIs (ownership transfer or shared ownership)
  virtual void async_write_move(std::vector<uint8_t>&& data) = 0;
  virtual void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) = 0;

  // Callbacks
  virtual void on_bytes(OnBytes cb) = 0;
  virtual void on_state(OnState cb) = 0;
  virtual void on_backpressure(OnBackpressure cb) = 0;
};
}  // namespace interface
}  // namespace unilink
