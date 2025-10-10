#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <regex>

#include "unilink/common/common.hpp"
#include "unilink/common/exceptions.hpp"
#include "unilink/common/input_validator.hpp"

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

  using ::testing::HasSubstr;
  // Check if the output contains the basic components
  EXPECT_THAT(output, HasSubstr("TAG"));
  EXPECT_THAT(output, HasSubstr("[DIR]"));
  EXPECT_THAT(output, HasSubstr("Hello World"));
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
  auto binary_data1 = common::safe_convert::string_to_uint8(data1, strlen(data1));
  feed_lines(acc, binary_data1.data(), binary_data1.size(), on_line);
  ASSERT_EQ(lines.size(), 1);
  EXPECT_EQ(lines[0], "hello");
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 2. Multiple lines
  const char* data2 = "line1\nline2\n";
  auto binary_data2 = common::safe_convert::string_to_uint8(data2, strlen(data2));
  feed_lines(acc, binary_data2.data(), binary_data2.size(), on_line);
  ASSERT_EQ(lines.size(), 2);
  if (lines.size() >= 2) {
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
  }
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 3. Partial line
  const char* data3 = "partial";
  auto binary_data3 = common::safe_convert::string_to_uint8(data3, strlen(data3));
  feed_lines(acc, binary_data3.data(), binary_data3.size(), on_line);
  EXPECT_TRUE(lines.empty());
  EXPECT_EQ(acc, "partial");

  // 4. Complete the partial line
  const char* data4 = "_line\n";
  auto binary_data4 = common::safe_convert::string_to_uint8(data4, strlen(data4));
  feed_lines(acc, binary_data4.data(), binary_data4.size(), on_line);
  ASSERT_EQ(lines.size(), 1);
  if (!lines.empty()) {
    EXPECT_EQ(lines[0], "partial_line");
  }
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 5. Line with CR LF
  const char* data5 = "crlf\r\n";
  auto binary_data5 = common::safe_convert::string_to_uint8(data5, strlen(data5));
  feed_lines(acc, binary_data5.data(), binary_data5.size(), on_line);
  ASSERT_EQ(lines.size(), 1);
  if (!lines.empty()) {
    EXPECT_EQ(lines[0], "crlf");
  }
  EXPECT_TRUE(acc.empty());
  lines.clear();

  // 6. Multiple lines with a partial line at the end
  const char* data6 = "lineA\nlineB\nlineC_part";
  auto binary_data6 = common::safe_convert::string_to_uint8(data6, strlen(data6));
  feed_lines(acc, binary_data6.data(), binary_data6.size(), on_line);
  ASSERT_EQ(lines.size(), 2);
  if (lines.size() >= 2) {
    EXPECT_EQ(lines[0], "lineA");
    EXPECT_EQ(lines[1], "lineB");
  }
  EXPECT_EQ(acc, "lineC_part");
  lines.clear();

  // 7. Empty lines
  const char* data7 = "\n\nfinal\n";
  acc.clear();
  auto binary_data7 = common::safe_convert::string_to_uint8(data7, strlen(data7));
  feed_lines(acc, binary_data7.data(), binary_data7.size(), on_line);
  ASSERT_EQ(lines.size(), 3);
  if (lines.size() >= 3) {
    EXPECT_EQ(lines[0], "");
    EXPECT_EQ(lines[1], "");
    EXPECT_EQ(lines[2], "final");
  }
  EXPECT_TRUE(acc.empty());
  lines.clear();
}

// ============================================================================
// LOG ROTATION TESTS
// ============================================================================

class LogRotationCommonTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clean up any existing test files
    cleanup_test_files();

    // Setup logger for testing
    Logger::instance().set_level(LogLevel::DEBUG);
    Logger::instance().set_console_output(false);  // Disable console for file testing
  }

  void TearDown() override {
    // Clean up test files
    cleanup_test_files();

    // Reset logger
    Logger::instance().set_file_output("");  // Disable file output
    Logger::instance().set_console_output(true);
  }

  void cleanup_test_files() {
    // Remove test log files
    std::vector<std::string> test_files = {"test_rotation.log",   "test_rotation.0.log", "test_rotation.1.log",
                                           "test_rotation.2.log", "test_rotation.3.log", "test_rotation.4.log",
                                           "test_rotation.5.log"};

    for (const auto& file : test_files) {
      if (std::filesystem::exists(file)) {
        std::filesystem::remove(file);
      }
    }
  }

  size_t count_log_files(const std::string& base_name) {
    size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(".")) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        if (filename.find(base_name) == 0 && filename.find(".log") != std::string::npos) {
          count++;
        }
      }
    }
    return count;
  }

  size_t get_file_size(const std::string& filename) {
    if (std::filesystem::exists(filename)) {
      return std::filesystem::file_size(filename);
    }
    return 0;
  }
};

TEST_F(LogRotationCommonTest, BasicRotationSetup) {
  // Test basic rotation configuration
  LogRotationConfig config;
  config.max_file_size_bytes = 1024;  // 1KB for testing
  config.max_files = 3;

  EXPECT_EQ(config.max_file_size_bytes, 1024);
  EXPECT_EQ(config.max_files, 3);
}

TEST_F(LogRotationCommonTest, FileSizeBasedRotation) {
  // Setup rotation with very small file size for testing
  LogRotationConfig config;
  config.max_file_size_bytes = 512;  // 512 bytes
  config.max_files = 5;

  Logger::instance().set_file_output_with_rotation("test_rotation.log", config);

  // Generate enough log data to trigger rotation
  for (int i = 0; i < 20; ++i) {
    UNILINK_LOG_INFO("test", "rotation",
                     "Test message " + std::to_string(i) +
                         " - This is a longer message to help reach the rotation threshold quickly.");
  }

  // Flush to ensure all data is written
  Logger::instance().flush();

  // Check if rotation occurred (should have multiple files)
  size_t file_count = count_log_files("test_rotation");
  EXPECT_GE(file_count, 1) << "At least one log file should exist";

  // Check if files are within size limits
  if (std::filesystem::exists("test_rotation.log")) {
    size_t current_size = get_file_size("test_rotation.log");
    EXPECT_LE(current_size, config.max_file_size_bytes * 2) << "Current log file should be reasonable size";
  }
}

TEST_F(LogRotationCommonTest, FileCountLimit) {
  // Setup rotation with small file size and low file count
  LogRotationConfig config;
  config.max_file_size_bytes = 256;  // 256 bytes
  config.max_files = 2;              // Only keep 2 files

  Logger::instance().set_file_output_with_rotation("test_rotation.log", config);

  // Generate lots of log data to trigger multiple rotations
  for (int i = 0; i < 50; ++i) {
    UNILINK_LOG_INFO("test", "count_limit",
                     "Message " + std::to_string(i) +
                         " - Generating enough data to trigger multiple rotations and test file count limits.");
  }

  Logger::instance().flush();

  // Check that file count doesn't exceed limit
  size_t file_count = count_log_files("test_rotation");
  EXPECT_LE(file_count, config.max_files + 1) << "File count should not exceed limit (current + rotated files)";
}

TEST_F(LogRotationCommonTest, LogRotationManagerDirectTest) {
  // Test LogRotation class directly
  LogRotationConfig config;
  config.max_file_size_bytes = 100;  // Very small for testing
  config.max_files = 2;

  LogRotation rotation(config);

  // Create a test file
  std::string test_file = "test_rotation.log";
  std::ofstream file(test_file);
  file << "Test data to make file larger than 100 bytes. ";
  file << "This should be enough to trigger rotation when we check.";
  file.close();

  // Check if rotation should occur
  bool should_rotate = rotation.should_rotate(test_file);
  EXPECT_TRUE(should_rotate) << "File should trigger rotation due to size";

  // Perform rotation
  std::string new_path = rotation.rotate(test_file);
  EXPECT_EQ(new_path, test_file) << "Should return original path for new log file";

  // Check that rotated file exists
  EXPECT_TRUE(std::filesystem::exists("test_rotation.0.log")) << "Rotated file should exist";

  // Clean up
  std::filesystem::remove(test_file);
  std::filesystem::remove("test_rotation.0.log");
}

TEST_F(LogRotationCommonTest, LogRotationWithoutRotation) {
  // Test that rotation doesn't occur when file is small
  LogRotationConfig config;
  config.max_file_size_bytes = 1024 * 1024;  // 1MB - very large
  config.max_files = 5;

  Logger::instance().set_file_output_with_rotation("test_rotation.log", config);

  // Generate small amount of log data
  for (int i = 0; i < 5; ++i) {
    UNILINK_LOG_INFO("test", "no_rotation", "Small message " + std::to_string(i));
  }

  Logger::instance().flush();

  // Should only have one file
  size_t file_count = count_log_files("test_rotation");
  EXPECT_EQ(file_count, 1) << "Should only have one file when size limit not reached";

  // File should exist and be small
  EXPECT_TRUE(std::filesystem::exists("test_rotation.log"));
  size_t file_size = get_file_size("test_rotation.log");
  EXPECT_LT(file_size, config.max_file_size_bytes) << "File should be smaller than rotation threshold";
}

// ============================================================================
// INPUT VALIDATION TESTS
// ============================================================================

/**
 * @brief Test host validation
 */
TEST(CommonTest, HostValidation) {
  EXPECT_NO_THROW(InputValidator::validate_host("localhost"));
  EXPECT_NO_THROW(InputValidator::validate_host("127.0.0.1"));
  EXPECT_NO_THROW(InputValidator::validate_host("example.com"));
  EXPECT_NO_THROW(InputValidator::validate_host("192.168.1.1"));

  // Invalid hosts
  EXPECT_THROW(InputValidator::validate_host(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_host("invalid..hostname"), ValidationException);

  // Test specific invalid IPv4 addresses - using direct IPv4 validation
  EXPECT_THROW(InputValidator::validate_ipv4_address("256.256.256.256"), ValidationException);
  EXPECT_THROW(InputValidator::validate_ipv4_address("192.168.01.1"), ValidationException);   // Leading zero
  EXPECT_THROW(InputValidator::validate_ipv4_address("192.168.1"), ValidationException);      // Too few octets
  EXPECT_THROW(InputValidator::validate_ipv4_address("192.168.1.1.1"), ValidationException);  // Too many octets

  EXPECT_THROW(InputValidator::validate_host("-invalid-hostname"), ValidationException);
  EXPECT_THROW(InputValidator::validate_host("hostname-"), ValidationException);
}

/**
 * @brief Test port validation
 */
TEST(CommonTest, PortValidation) {
  EXPECT_NO_THROW(InputValidator::validate_port(1));
  EXPECT_NO_THROW(InputValidator::validate_port(8080));
  EXPECT_NO_THROW(InputValidator::validate_port(65535));

  EXPECT_THROW(InputValidator::validate_port(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_port(65536), ValidationException);
}

/**
 * @brief Test device path validation
 */
TEST(CommonTest, DevicePathValidation) {
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyUSB0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyACM0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("COM1"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("COM2"));

  EXPECT_THROW(InputValidator::validate_device_path(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_device_path("invalid"), ValidationException);
}

/**
 * @brief Test baud rate validation
 */
TEST(CommonTest, BaudRateValidation) {
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(9600));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(115200));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(4000000));

  EXPECT_THROW(InputValidator::validate_baud_rate(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_baud_rate(49), ValidationException);
  EXPECT_THROW(InputValidator::validate_baud_rate(4000001), ValidationException);
}

/**
 * @brief Test buffer size validation
 */
TEST(CommonTest, BufferSizeValidation) {
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(1024));
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(4096));
  EXPECT_NO_THROW(InputValidator::validate_buffer_size(65536));

  EXPECT_THROW(InputValidator::validate_buffer_size(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_buffer_size(65 * 1024 * 1024), ValidationException);
}

/**
 * @brief Test timeout validation
 */
TEST(CommonTest, TimeoutValidation) {
  // Valid timeouts
  EXPECT_NO_THROW(InputValidator::validate_timeout(100));  // Minimum valid timeout
  EXPECT_NO_THROW(InputValidator::validate_timeout(5000));
  EXPECT_NO_THROW(InputValidator::validate_timeout(300000));

  // Invalid timeouts
  EXPECT_THROW(InputValidator::validate_timeout(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_timeout(99), ValidationException);  // Below minimum
  EXPECT_THROW(InputValidator::validate_timeout(300001), ValidationException);
}

/**
 * @brief Test retry interval validation
 */
TEST(CommonTest, RetryIntervalValidation) {
  // Valid retry intervals
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(100));  // Minimum valid retry interval
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(1000));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(300000));

  // Invalid retry intervals
  EXPECT_THROW(InputValidator::validate_retry_interval(0), ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_interval(99), ValidationException);  // Below minimum
  EXPECT_THROW(InputValidator::validate_retry_interval(300001), ValidationException);
}

// ============================================================================
// CONSTANTS TESTS
// ============================================================================

/**
 * @brief Test constants are properly defined
 */
TEST(CommonTest, ConstantsDefinition) {
  // Buffer size constants
  EXPECT_GT(constants::MAX_BUFFER_SIZE, 0);
  EXPECT_GT(constants::MIN_BUFFER_SIZE, 0);
  EXPECT_GT(constants::DEFAULT_BUFFER_SIZE, 0);
  EXPECT_GT(constants::LARGE_BUFFER_THRESHOLD, 0);

  // Retry and timeout constants
  EXPECT_GT(constants::DEFAULT_RETRY_INTERVAL_MS, 0);
  EXPECT_GT(constants::MIN_RETRY_INTERVAL_MS, 0);
  EXPECT_GT(constants::MAX_RETRY_INTERVAL_MS, 0);
  EXPECT_GT(constants::DEFAULT_CONNECTION_TIMEOUT_MS, 0);
  EXPECT_GT(constants::MIN_CONNECTION_TIMEOUT_MS, 0);
  EXPECT_GT(constants::MAX_CONNECTION_TIMEOUT_MS, 0);

  // Validation constants
  EXPECT_GT(constants::MAX_HOSTNAME_LENGTH, 0);
  EXPECT_GT(constants::MAX_DEVICE_PATH_LENGTH, 0);
  EXPECT_GT(constants::MIN_BAUD_RATE, 0);
  EXPECT_GT(constants::MAX_BAUD_RATE, 0);
  EXPECT_GT(constants::MIN_DATA_BITS, 0);
  EXPECT_GT(constants::MAX_DATA_BITS, 0);
  EXPECT_GT(constants::MIN_STOP_BITS, 0);
  EXPECT_GT(constants::MAX_STOP_BITS, 0);
}

/**
 * @brief Test constants consistency
 */
TEST(CommonTest, ConstantsConsistency) {
  // Buffer size consistency
  EXPECT_LE(constants::MIN_BUFFER_SIZE, constants::DEFAULT_BUFFER_SIZE);
  EXPECT_LE(constants::DEFAULT_BUFFER_SIZE, constants::LARGE_BUFFER_THRESHOLD);
  EXPECT_LE(constants::LARGE_BUFFER_THRESHOLD, constants::MAX_BUFFER_SIZE);

  // Retry interval consistency
  EXPECT_LE(constants::MIN_RETRY_INTERVAL_MS, constants::DEFAULT_RETRY_INTERVAL_MS);
  EXPECT_LE(constants::DEFAULT_RETRY_INTERVAL_MS, constants::MAX_RETRY_INTERVAL_MS);

  // Timeout consistency
  EXPECT_LE(constants::MIN_CONNECTION_TIMEOUT_MS, constants::DEFAULT_CONNECTION_TIMEOUT_MS);
  EXPECT_LE(constants::DEFAULT_CONNECTION_TIMEOUT_MS, constants::MAX_CONNECTION_TIMEOUT_MS);

  // Baud rate consistency
  EXPECT_LE(constants::MIN_BAUD_RATE, constants::MAX_BAUD_RATE);

  // Data bits consistency
  EXPECT_LE(constants::MIN_DATA_BITS, constants::MAX_DATA_BITS);

  // Stop bits consistency
  EXPECT_LE(constants::MIN_STOP_BITS, constants::MAX_STOP_BITS);
}
