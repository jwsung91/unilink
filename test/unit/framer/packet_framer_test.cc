#include "unilink/framer/packet_framer.hpp"

#include <gtest/gtest.h>

#include <vector>

using namespace unilink;
using namespace unilink::framer;

class PacketFramerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    start_ = {'S', 'T'};
    end_ = {'E', 'N'};
    framer_ = std::make_unique<PacketFramer>(start_, end_, 1024);
    framer_->set_on_message([this](memory::ConstByteSpan msg) {
      std::vector<uint8_t> v(msg.begin(), msg.end());
      messages_.push_back(v);
    });
  }

  std::vector<uint8_t> start_;
  std::vector<uint8_t> end_;
  std::unique_ptr<PacketFramer> framer_;
  std::vector<std::vector<uint8_t>> messages_;
};

TEST_F(PacketFramerTest, SimplePacket) {
  std::vector<uint8_t> data = {'S', 'T', 'D', 'A', 'T', 'A', 'E', 'N'};
  framer_->push_bytes(memory::ConstByteSpan(data.data(), data.size()));
  ASSERT_EQ(messages_.size(), 1);
  EXPECT_EQ(messages_[0], data);
}

TEST_F(PacketFramerTest, SyncGarbage) {
  std::vector<uint8_t> garbage = {'X', 'Y'};
  std::vector<uint8_t> packet = {'S', 'T', 'D', 'E', 'N'};
  std::vector<uint8_t> data = garbage;
  data.insert(data.end(), packet.begin(), packet.end());

  framer_->push_bytes(memory::ConstByteSpan(data.data(), data.size()));
  ASSERT_EQ(messages_.size(), 1);
  EXPECT_EQ(messages_[0], packet);
}

TEST_F(PacketFramerTest, SplitPacket) {
  std::vector<uint8_t> part1 = {'S', 'T', 'D'};
  std::vector<uint8_t> part2 = {'A', 'E', 'N'};
  framer_->push_bytes(memory::ConstByteSpan(part1.data(), part1.size()));
  ASSERT_EQ(messages_.size(), 0);
  framer_->push_bytes(memory::ConstByteSpan(part2.data(), part2.size()));
  ASSERT_EQ(messages_.size(), 1);
  std::vector<uint8_t> expected = {'S', 'T', 'D', 'A', 'E', 'N'};
  EXPECT_EQ(messages_[0], expected);
}

TEST_F(PacketFramerTest, MergedPackets) {
  std::vector<uint8_t> p1 = {'S', 'T', '1', 'E', 'N'};
  std::vector<uint8_t> p2 = {'S', 'T', '2', 'E', 'N'};
  std::vector<uint8_t> data = p1;
  data.insert(data.end(), p2.begin(), p2.end());

  framer_->push_bytes(memory::ConstByteSpan(data.data(), data.size()));
  ASSERT_EQ(messages_.size(), 2);
  EXPECT_EQ(messages_[0], p1);
  EXPECT_EQ(messages_[1], p2);
}

TEST_F(PacketFramerTest, MaxLengthExceeded) {
  // Max len 6. "ST12EN" = 6.
  framer_ = std::make_unique<PacketFramer>(start_, end_, 6);
  framer_->set_on_message([this](memory::ConstByteSpan msg) {
    std::vector<uint8_t> v(msg.begin(), msg.end());
    messages_.push_back(v);
  });

  // Too long: "ST123EN" = 7
  std::vector<uint8_t> bad = {'S', 'T', '1', '2', '3', 'E', 'N'};
  framer_->push_bytes(memory::ConstByteSpan(bad.data(), bad.size()));
  ASSERT_EQ(messages_.size(), 0);

  // Valid: "ST1EN" = 5
  std::vector<uint8_t> valid = {'S', 'T', '1', 'E', 'N'};
  framer_->push_bytes(memory::ConstByteSpan(valid.data(), valid.size()));
  ASSERT_EQ(messages_.size(), 1);
  EXPECT_EQ(messages_[0], valid);
}

TEST_F(PacketFramerTest, RejectEmptyPatterns) {
  std::vector<uint8_t> empty_start;
  std::vector<uint8_t> empty_end;
  EXPECT_THROW({ PacketFramer(empty_start, empty_end, 1024); }, std::invalid_argument);
}
