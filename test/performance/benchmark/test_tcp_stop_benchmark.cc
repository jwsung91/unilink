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
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <thread>
#include <iostream>
#include <numeric>
#include <vector>

#include "unilink/wrapper/tcp_client/tcp_client.hpp"
#include "test_utils.hpp"

using namespace unilink;
using namespace unilink::test;

class TcpClientStopBenchmark : public ::testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
  }
};

TEST_F(TcpClientStopBenchmark, StopLatency) {
  std::cout << "\n=== TcpClient Stop Latency Benchmark ===" << std::endl;

  const int iterations = 10;
  std::vector<double> latencies;
  latencies.reserve(iterations);

  for (int i = 0; i < iterations; ++i) {
    auto ioc = std::make_shared<boost::asio::io_context>();
    wrapper::TcpClient client("127.0.0.1", 12345, ioc);

    // Configure to manage external context
    client.set_manage_external_context(true);

    // Start (creates thread and work guard)
    client.start();

    // Allow it to start up
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Measure stop time
    auto start = std::chrono::high_resolution_clock::now();
    client.stop();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    latencies.push_back(duration / 1000.0); // ms

    std::cout << "Iteration " << i + 1 << ": " << (duration / 1000.0) << " ms" << std::endl;
  }

  double total = std::accumulate(latencies.begin(), latencies.end(), 0.0);
  double avg = total / iterations;

  std::cout << "Average Stop Latency: " << avg << " ms" << std::endl;

  // Baseline expectation: around 100ms due to sleep
  // We expect this to fail if we set a tight limit before optimization
  // But for now we just report it.
}
