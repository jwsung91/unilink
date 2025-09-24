#include <gtest/gtest.h>

#include <regex>

#include "unilink/common/common.hpp"

using namespace unilink::common;

TEST(CommonTest, LinkStateToString) {
  EXPECT_STREQ("Idle", to_cstr(LinkState::Idle));
  EXPECT_STREQ("Connecting", to_cstr(LinkState::Connecting));
  EXPECT_STREQ("Listening", to_cstr(LinkState::Listening));
  EXPECT_STREQ("Connected", to_cstr(LinkState::Connected));
  EXPECT_STREQ("Closed", to_cstr(LinkState::Closed));
  EXPECT_STREQ("Error", to_cstr(LinkState::Error));
}

TEST(CommonTest, TimestampFormat) {
  std::string ts = ts_now();
  // YYYY-MM-DD HH:MM:SS.ms
  std::regex re(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})");
  EXPECT_TRUE(std::regex_match(ts, re));
}

class LogMessageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Redirect std::cout to our stringstream
    sbuf_ = std::cout.rdbuf();
    std::cout.rdbuf(oss_.rdbuf());
  }

  void TearDown() override {
    // Restore original std::cout
    std::cout.rdbuf(sbuf_);
  }

  std::stringstream oss_;
  std::streambuf* sbuf_;
};

TEST_F(LogMessageTest, BasicLogging) {
  log_message("TAG", "DIR", "Hello World");
  std::string output = oss_.str();

  // Check if the output contains the basic components
  EXPECT_NE(output.find("[TAG]"), std::string::npos);
  EXPECT_NE(output.find("[DIR]"), std::string::npos);
  EXPECT_NE(output.find("Hello World"), std::string::npos);
  // Check for trailing newline
  EXPECT_EQ(output.back(), '\n');
}

TEST_F(LogMessageTest, RemovesTrailingNewline) {
  log_message("TAG", "DIR", "Message with newline\n");
  std::string output = oss_.str();

  // The original newline in the message should be stripped,
  // and log_message should add its own.
  // So, we check that "Message with newline\n\n" is not present.
  EXPECT_EQ(output.find("Message with newline\n\n"), std::string::npos);
  EXPECT_NE(output.find("Message with newline\n"), std::string::npos);
}

TEST(CommonTest, FeedLines) {
  std::string acc;
  std::vector<std::string> lines;
  auto on_line = [&](std::string line) { lines.push_back(line); };

  // 1. Single complete line
  const char* data1 = "hello\n";
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data1), strlen(data1),
             on_line);
  ASSERT_EQ(lines.size(), 1);
  EXPECT_EQ(lines[0], "hello");
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 2. Multiple lines
  const char* data2 = "line1\nline2\n";
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data2), strlen(data2),
             on_line);
  ASSERT_EQ(lines.size(), 2);
  EXPECT_EQ(lines[0], "line1");
  EXPECT_EQ(lines[1], "line2");
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 3. Partial line
  const char* data3 = "partial";
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data3), strlen(data3),
             on_line);
  EXPECT_TRUE(lines.empty());
  EXPECT_EQ(acc, "partial");

  // 4. Complete the partial line
  const char* data4 = "_line\n";
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data4), strlen(data4),
             on_line);
  ASSERT_EQ(lines.size(), 1);
  EXPECT_EQ(lines[0], "partial_line");
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 5. Line with CR LF
  const char* data5 = "crlf\r\n";
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data5), strlen(data5),
             on_line);
  ASSERT_EQ(lines.size(), 1);
  EXPECT_EQ(lines[0], "crlf");
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 6. Multiple lines with a partial line at the end
  const char* data6 = "lineA\nlineB\nlineC_part";
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data6), strlen(data6),
             on_line);
  ASSERT_EQ(lines.size(), 2);
  EXPECT_EQ(lines[0], "lineA");
  EXPECT_EQ(lines[1], "lineB");
  EXPECT_EQ(acc, "lineC_part");
  lines.clear();

  // 7. Empty lines
  const char* data7 = "\n\nfinal\n";
  acc.clear();
  feed_lines(acc, reinterpret_cast<const uint8_t*>(data7), strlen(data7),
             on_line);
  ASSERT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "");
  EXPECT_EQ(lines[1], "");
  EXPECT_EQ(lines[2], "final");
  EXPECT_TRUE(acc.empty());
  lines.clear();
}
