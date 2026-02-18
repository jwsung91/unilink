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
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

class TcpCallbackBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {
    port_ = TestUtils::getAvailableTestPort();
    server_ = tcp_server(port_).build();
    client_ = tcp_client("127.0.0.1", port_).build();
    auto f1 = server_->start();
    auto f2 = client_->start();
    f1.get();
    f2.get();  // Wait for full startup
    TestUtils::waitForCondition([&]() { return client_->is_connected(); }, 5000);
  }

  void TearDown() override {
    client_->stop();
    server_->stop();
  }

  uint16_t port_;
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
};

TEST_F(TcpCallbackBenchmark, OnDataPerformance) {
  std::atomic<size_t> bytes_received{0};
  const size_t target_bytes = 10 * 1024 * 1024;  // 10MB

  client_->on_data([&](const wrapper::MessageContext& ctx) { bytes_received += ctx.data().size(); });

  std::string chunk(64 * 1024, 'X');
  auto start = std::chrono::high_resolution_clock::now();

  while (bytes_received < target_bytes) {
    server_->broadcast(chunk);
    std::this_thread::yield();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "10MB processed in " << duration << "ms" << std::endl;
}
