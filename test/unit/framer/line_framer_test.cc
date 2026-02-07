#include "unilink/framer/line_framer.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace unilink;
using namespace unilink::framer;

class LineFramerTest : public ::testing::Test {
protected:
    void SetUp() override {
        framer_ = std::make_unique<LineFramer>("\n", false, 1024);
        framer_->set_on_message([this](memory::ConstByteSpan msg) {
            std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
            messages_.push_back(s);
        });
    }

    std::unique_ptr<LineFramer> framer_;
    std::vector<std::string> messages_;
};

TEST_F(LineFramerTest, SingleMessage) {
    std::string data = "Hello\n";
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    ASSERT_EQ(messages_.size(), 1);
    EXPECT_EQ(messages_[0], "Hello");
}

TEST_F(LineFramerTest, SplitMessage) {
    std::string part1 = "He";
    std::string part2 = "llo\n";
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(part1.data()), part1.size()));
    ASSERT_EQ(messages_.size(), 0);
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(part2.data()), part2.size()));
    ASSERT_EQ(messages_.size(), 1);
    EXPECT_EQ(messages_[0], "Hello");
}

TEST_F(LineFramerTest, MergedMessages) {
    std::string data = "Msg1\nMsg2\n";
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    ASSERT_EQ(messages_.size(), 2);
    EXPECT_EQ(messages_[0], "Msg1");
    EXPECT_EQ(messages_[1], "Msg2");
}

TEST_F(LineFramerTest, IncludeDelimiter) {
    framer_ = std::make_unique<LineFramer>("\n", true, 1024);
    framer_->set_on_message([this](memory::ConstByteSpan msg) {
        std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
        messages_.push_back(s);
    });

    std::string data = "Hello\n";
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    ASSERT_EQ(messages_.size(), 1);
    EXPECT_EQ(messages_[0], "Hello\n");
}

TEST_F(LineFramerTest, MaxLengthReset) {
    // Max length 5. "12345" is ok. "123456" triggers reset.
    framer_ = std::make_unique<LineFramer>("\n", false, 5);
    framer_->set_on_message([this](memory::ConstByteSpan msg) {
        std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
        messages_.push_back(s);
    });

    std::string data = "123456"; // No delimiter, exceeds max
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    // Should have cleared buffer.

    // Now send valid message
    std::string valid = "Hi\n";
    framer_->push_bytes(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(valid.data()), valid.size()));

    ASSERT_EQ(messages_.size(), 1);
    EXPECT_EQ(messages_[0], "Hi");
}
