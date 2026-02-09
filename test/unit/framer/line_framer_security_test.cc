#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "unilink/framer/line_framer.hpp"

using namespace unilink;
using namespace unilink::framer;

class LineFramerSecurityTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Max length 1024, newline delimiter
    framer_ = std::make_unique<LineFramer>("\n", false, 1024);
    framer_->set_on_message([this](memory::ConstByteSpan msg) {
      std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
      messages_.push_back(s);
    });
  }

  std::unique_ptr<LineFramer> framer_;
  std::vector<std::string> messages_;
};

TEST_F(LineFramerSecurityTest, LargeChunkProcessing) {
  // Create a payload larger than max_length but consisting of valid short lines
  std::string large_payload;
  large_payload.reserve(2048);
  int expected_count = 0;

  for (int i = 0; i < 100; ++i) {
    std::string line = "Line" + std::to_string(i) + "\n";
    if (large_payload.size() + line.size() > 2000) break;
    large_payload += line;
    expected_count++;
  }

  // Pass the entire payload at once. Total size > 1024 (max_length).
  // The framer should process all lines correctly without clearing the buffer due to overflow.
  framer_->push_bytes(
      memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(large_payload.data()), large_payload.size()));

  ASSERT_EQ(messages_.size(), expected_count);
  EXPECT_EQ(messages_[0], "Line0");
  EXPECT_EQ(messages_.back(), "Line" + std::to_string(expected_count - 1));
}

TEST_F(LineFramerSecurityTest, HugeLineRejection) {
  // Construct: "Valid1\n" + (2000 chars line) + "\n" + "Valid2\n"
  std::string valid1 = "Valid1\n";
  std::string huge_line(2000, 'A');
  huge_line += "\n";
  std::string valid2 = "Valid2\n";

  std::string payload = valid1 + huge_line + valid2;

  // Current implementation would accept huge_line!
  // New implementation should reject it.
  framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

  // Check that at least valid lines are processed.
  // The huge line should be dropped.
  bool found_valid1 = false;
  bool found_valid2 = false;
  bool found_huge = false;

  for (const auto& msg : messages_) {
    if (msg == "Valid1") found_valid1 = true;
    if (msg == "Valid2") found_valid2 = true;
    if (msg.length() >= 2000) found_huge = true;
  }

  EXPECT_TRUE(found_valid1) << "Valid1 not found";
  EXPECT_FALSE(found_huge) << "Huge line should be rejected";
  // Depending on implementation, Valid2 might be dropped if we clear buffer and return?
  // My proposed implementation continues processing.
  EXPECT_TRUE(found_valid2) << "Valid2 not found";
}

TEST_F(LineFramerSecurityTest, SplitDelimiter) {
  // Delimiter "\r\n"
  framer_ = std::make_unique<LineFramer>("\r\n", false, 1024);
  framer_->set_on_message([this](memory::ConstByteSpan msg) {
    std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
    messages_.push_back(s);
  });

  std::string part1 = "Hello\r";
  std::string part2 = "\nWorld\r\n";

  framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(part1.data()), part1.size()));
  ASSERT_EQ(messages_.size(), 0);

  framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(part2.data()), part2.size()));
  ASSERT_EQ(messages_.size(), 2);
  EXPECT_EQ(messages_[0], "Hello");
  EXPECT_EQ(messages_[1], "World");
}
