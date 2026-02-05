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

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "unilink/interface/channel.hpp"
#include "unilink/wrapper/serial/serial.hpp"

namespace unilink {
namespace performance {

class MockChannel : public interface::Channel {
 public:
  void start() override {}
  void stop() override {}
  bool is_connected() const override { return true; }

  void async_write_copy(const uint8_t*, size_t) override {}
  void async_write_move(std::vector<uint8_t>&&) override {}
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>>) override {}

  void on_bytes(OnBytes) override {}
  void on_state(OnState) override {}
  void on_backpressure(OnBackpressure) override {}
};

TEST(SerialStopPerf, StopDuration) {
  auto channel = std::make_shared<MockChannel>();
  wrapper::Serial serial(channel);

  serial.start();  // Should be instant with mock

  auto start_time = std::chrono::high_resolution_clock::now();
  serial.stop();
  auto end_time = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  std::cout << "Serial::stop() took " << duration << " ms" << std::endl;

  // Expectation for optimization: should be very fast (< 10ms)
  EXPECT_LT(duration, 10);
}

}  // namespace performance
}  // namespace unilink
