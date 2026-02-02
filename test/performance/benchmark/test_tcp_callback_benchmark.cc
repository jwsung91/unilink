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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "unilink/interface/channel.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"

using namespace unilink;

class MockChannel : public interface::Channel {
 public:
  void start() override {}
  void stop() override {}
  bool is_connected() const override { return true; }

  void async_write_copy(const uint8_t* data, size_t size) override {}
  void async_write_move(std::vector<uint8_t>&& data) override {}
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override {}

  void on_bytes(OnBytes cb) override { on_bytes_ = cb; }
  void on_state(OnState cb) override {}
  void on_backpressure(OnBackpressure cb) override {}

  void trigger_bytes(const uint8_t* data, size_t size) {
    if (on_bytes_) {
      on_bytes_(data, size);
    }
  }

 private:
  OnBytes on_bytes_;
};

class TcpCallbackBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_channel_ = std::make_shared<MockChannel>();
    // Inject mock channel into TcpClient
    client_ = std::make_unique<wrapper::TcpClient>(mock_channel_);
  }

  std::shared_ptr<MockChannel> mock_channel_;
  std::unique_ptr<wrapper::TcpClient> client_;
};

TEST_F(TcpCallbackBenchmark, OnDataPerformance) {
  const int iterations = 1000000;
  const size_t data_size = 1024;
  std::vector<uint8_t> buffer(data_size, 'A');

  volatile size_t bytes_received = 0;
  client_->on_data([&](const std::string& data) {
    bytes_received += data.size();
  });

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    mock_channel_->trigger_bytes(buffer.data(), buffer.size());
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(iterations) / (duration.count() / 1000000.0);

  std::cout << "OnData (String Conversion) Performance:" << std::endl;
  std::cout << "  Iterations: " << iterations << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
}

TEST_F(TcpCallbackBenchmark, OnBytesPerformance) {
  const int iterations = 1000000;
  const size_t data_size = 1024;
  std::vector<uint8_t> buffer(data_size, 'A');

  volatile size_t bytes_received = 0;
  client_->on_bytes([&](const uint8_t* data, size_t size) {
    bytes_received += size;
  });

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    mock_channel_->trigger_bytes(buffer.data(), buffer.size());
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(iterations) / (duration.count() / 1000000.0);

  std::cout << "OnBytes (Zero Copy) Performance:" << std::endl;
  std::cout << "  Iterations: " << iterations << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
}
