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

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "test/utils/test_utils.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/wrapper/serial/serial.hpp"

using namespace unilink;
using namespace unilink::test;

namespace {

class DummyChannel : public interface::Channel {
 public:
  void start() override {
    started_ = true;
    if (on_state_) on_state_(common::LinkState::Connected);
  }

  void stop() override {
    stopped_ = true;
    if (on_state_) on_state_(common::LinkState::Closed);
  }

  bool is_connected() const override { return started_ && !stopped_; }

  void async_write_copy(const uint8_t* /*data*/, size_t /*size*/) override {}
  void async_write_move(std::vector<uint8_t>&& /*data*/) override {}
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> /*data*/) override {}

  void on_bytes(OnBytes cb) override { on_bytes_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) override { on_bp_ = std::move(cb); }

 private:
  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  bool started_{false};
  bool stopped_{false};
};

}  // namespace

TEST(SerialWrapperAdvancedTest, AutoManageStartsAndStopsChannel) {
  auto dummy_channel = std::make_shared<DummyChannel>();
  auto serial = std::make_shared<wrapper::Serial>(dummy_channel);

  std::atomic<bool> connected{false};
  std::atomic<bool> disconnected{false};

  serial->on_connect([&]() { connected = true; });
  serial->on_disconnect([&]() { disconnected = true; });

  serial->auto_manage(true);

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return connected.load(); }, 500));

  serial->stop();

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return disconnected.load(); }, 500));
}
