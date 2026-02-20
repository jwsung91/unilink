/*
 * Copyright 2025 Bolt
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
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "unilink/framer/line_framer.hpp"

using namespace unilink;
using namespace unilink::framer;

class LineFramerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(LineFramerPerfTest, ThroughputLargeChunk) {
  const size_t total_size = 50 * 1024 * 1024;  // 50 MB
  const std::string message = "Hello, world! This is a test message.\n";
  std::vector<uint8_t> data;
  data.reserve(total_size + message.size());

  while (data.size() < total_size) {
    data.insert(data.end(), message.begin(), message.end());
  }

  LineFramer framer;
  volatile size_t msg_count = 0;
  framer.set_on_message([&](memory::ConstByteSpan) { msg_count++; });

  auto start = std::chrono::high_resolution_clock::now();

  // Push all data at once
  framer.push_bytes(memory::ConstByteSpan(data));

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  double throughput_mb = (data.size() / (1024.0 * 1024.0)) / diff.count();

  std::cout << "LineFramer Large Chunk Throughput: " << throughput_mb << " MB/s" << std::endl;
  std::cout << "Processed " << msg_count << " messages in " << diff.count() << " seconds." << std::endl;
}
