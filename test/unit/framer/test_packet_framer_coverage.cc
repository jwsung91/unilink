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

#include "unilink/framer/packet_framer.hpp"

using namespace unilink;

namespace unilink {
namespace test {

static constexpr size_t DEFAULT_MAX = 65535;

TEST(PacketFramerCoverageTest, EmptyDataInput) {
  framer::PacketFramer framer({0x01}, {0x02}, DEFAULT_MAX);
  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });
  framer.push_bytes(memory::ConstByteSpan(nullptr, 0));
  EXPECT_EQ(count, 0);
}

TEST(PacketFramerCoverageTest, FastPathEmptyEndPattern) {
  framer::PacketFramer framer({0x01}, {}, DEFAULT_MAX);
  std::vector<uint8_t> messages;
  framer.on_message([&](memory::ConstByteSpan data) { messages.insert(messages.end(), data.begin(), data.end()); });

  std::vector<uint8_t> input = {0x01, 0x01, 0x01};
  framer.push_bytes(memory::ConstByteSpan(input.data(), input.size()));
  EXPECT_EQ(messages.size(), 3);
}

TEST(PacketFramerCoverageTest, FastPathMaxLengthExceeded) {
  framer::PacketFramer framer({0x01}, {0x02}, 5);  // max 5
  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });

  // Packet: 01 00 00 00 00 02 (length 6) -> exceeds 5
  std::vector<uint8_t> input = {0x01, 0x00, 0x00, 0x00, 0x00, 0x02};
  framer.push_bytes(memory::ConstByteSpan(input.data(), input.size()));
  EXPECT_EQ(count, 0);
}

TEST(PacketFramerCoverageTest, FastPathPartialMatchAtEnd) {
  framer::PacketFramer framer({0x01, 0x02}, {0x03}, DEFAULT_MAX);
  std::vector<uint8_t> input = {0x00, 0x01};  // 0x01 is partial start match
  framer.push_bytes(memory::ConstByteSpan(input.data(), input.size()));

  // Now push the rest
  input = {0x02, 0x00, 0x03};  // Completes 01 02 ... 03
  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });
  framer.push_bytes(memory::ConstByteSpan(input.data(), input.size()));
  EXPECT_EQ(count, 1);
}

TEST(PacketFramerCoverageTest, BufferPathEmptyStartPattern) {
  framer::PacketFramer framer({}, {0x02}, DEFAULT_MAX);  // empty start
  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });

  // First push partial to trigger buffer path
  std::vector<uint8_t> input1 = {0x01};
  framer.push_bytes(memory::ConstByteSpan(input1.data(), input1.size()));

  // Then push end
  std::vector<uint8_t> input2 = {0x02};
  framer.push_bytes(memory::ConstByteSpan(input2.data(), input2.size()));
  EXPECT_EQ(count, 1);
}

TEST(PacketFramerCoverageTest, BufferPathStartPatternNotFoundPartialKeep) {
  framer::PacketFramer framer({0x01, 0x02}, {0x03}, DEFAULT_MAX);
  // Buffer path: push data that does not contain start pattern but ends with partial
  std::vector<uint8_t> input1 = {0x00, 0x00};  // Force buffer path
  framer.push_bytes(memory::ConstByteSpan(input1.data(), input1.size()));

  std::vector<uint8_t> input2 = {0x05, 0x05, 0x01};  // Ends with partial 0x01
  framer.push_bytes(memory::ConstByteSpan(input2.data(), input2.size()));

  // Next push completes start pattern
  std::vector<uint8_t> input3 = {0x02, 0x03};
  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });
  framer.push_bytes(memory::ConstByteSpan(input3.data(), input3.size()));
  EXPECT_EQ(count, 1);
}

TEST(PacketFramerCoverageTest, BufferPathCollectEmptyEndPattern) {
  framer::PacketFramer framer({0x01}, {}, DEFAULT_MAX);
  // Force buffer path by pushing partial match or something
  std::vector<uint8_t> input1 = {0x05};
  framer.push_bytes(memory::ConstByteSpan(input1.data(), input1.size()));

  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });
  std::vector<uint8_t> input2 = {0x01, 0x01};
  framer.push_bytes(memory::ConstByteSpan(input2.data(), input2.size()));
  EXPECT_EQ(count, 2);
}

TEST(PacketFramerCoverageTest, BufferPathCollectMaxLengthExceeded) {
  framer::PacketFramer framer({0x01}, {0x02}, 5);
  // Push start
  std::vector<uint8_t> input1 = {0x01, 0x00, 0x00};
  framer.push_bytes(memory::ConstByteSpan(input1.data(), input1.size()));

  // Push more to exceed max_length while collecting
  std::vector<uint8_t> input2 = {0x00, 0x00, 0x00, 0x00};
  framer.push_bytes(memory::ConstByteSpan(input2.data(), input2.size()));
  // Should have reset
}

TEST(PacketFramerCoverageTest, InvalidConstructorArgs) {
  EXPECT_THROW(framer::PacketFramer({}, {}, DEFAULT_MAX), std::invalid_argument);
}

TEST(PacketFramerCoverageTest, ResetClearsState) {
  framer::PacketFramer framer({0x01}, {0x02}, DEFAULT_MAX);
  framer.push_bytes(memory::ConstByteSpan(std::vector<uint8_t>{0x01, 0x00}.data(), 2));
  framer.reset();
  // After reset, pushing 0x02 should NOT trigger message because 0x01 was cleared
  int count = 0;
  framer.on_message([&](memory::ConstByteSpan) { count++; });
  framer.push_bytes(memory::ConstByteSpan(std::vector<uint8_t>{0x02}.data(), 1));
  EXPECT_EQ(count, 0);
}

}  // namespace test
}  // namespace unilink
