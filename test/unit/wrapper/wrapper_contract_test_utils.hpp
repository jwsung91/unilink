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

#include <memory>
#include <string_view>
#include <vector>

#include "unilink/interface/channel.hpp"

namespace unilink::test::wrapper_support {

class FakeChannel : public interface::Channel {
 public:
  void start() override { connected_ = true; }
  void stop() override { connected_ = false; }
  bool is_connected() const override { return connected_; }

  void async_write_copy(memory::ConstByteSpan) override {}
  void async_write_move(std::vector<uint8_t>&&) override {}
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>>) override {}

  void on_bytes(OnBytes cb) override { on_bytes_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) override { on_backpressure_ = std::move(cb); }

  void emit_bytes(std::string_view text) {
    if (!on_bytes_) return;
    on_bytes_(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(text.data()), text.size()));
  }

  void emit_state(base::LinkState state) {
    if (state == base::LinkState::Connected) {
      connected_ = true;
    } else if (state == base::LinkState::Closed || state == base::LinkState::Error || state == base::LinkState::Idle) {
      connected_ = false;
    }

    if (on_state_) {
      on_state_(state);
    }
  }

 private:
  bool connected_{false};
  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_backpressure_;
};

}  // namespace unilink::test::wrapper_support
