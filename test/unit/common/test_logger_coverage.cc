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

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "test_utils.hpp"
#include "unilink/common/logger.hpp"

using namespace unilink::common;
using namespace std::chrono_literals;
using unilink::test::TestUtils;

/**
 * @brief Logger Coverage Test
 */
class LoggerCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create unique log file for each test to avoid parallel execution conflicts
    const ::testing::TestInfo* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string test_name = test_info->name();

    // Generate unique filename with timestamp and random suffix
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::random_device rd;
    std::string file_name = "unilink_logger_test_" + test_name + "_" + std::to_string(now) + "_" +
                            std::to_string(rd()) + ".log";
    test_log_file_ = TestUtils::makeTempFilePath(file_name);
    TestUtils::removeFileIfExists(test_log_file_);
  }

  void TearDown() override { TestUtils::removeFileIfExists(test_log_file_); }

  std::filesystem::path test_log_file_;
};

// ============================================================================
// LOG LEVEL TESTS
// ============================================================================

TEST_F(LoggerCoverageTest, SetAndGetLogLevel) {
  Logger::instance().set_level(LogLevel::DEBUG);
  EXPECT_EQ(Logger::instance().get_level(), LogLevel::DEBUG);

  Logger::instance().set_level(LogLevel::INFO);
  EXPECT_EQ(Logger::instance().get_level(), LogLevel::INFO);

  Logger::instance().set_level(LogLevel::WARNING);
  EXPECT_EQ(Logger::instance().get_level(), LogLevel::WARNING);

  Logger::instance().set_level(LogLevel::ERROR);
  EXPECT_EQ(Logger::instance().get_level(), LogLevel::ERROR);
}

TEST_F(LoggerCoverageTest, LogLevelFiltering) {
  Logger::instance().set_level(LogLevel::WARNING);

  // These should be filtered out
  UNILINK_LOG_DEBUG("test", "operation", "debug message");
  UNILINK_LOG_INFO("test", "operation", "info message");

  // These should pass
  UNILINK_LOG_WARNING("test", "operation", "warning message");
  UNILINK_LOG_ERROR("test", "operation", "error message");
}

// ============================================================================
// FILE LOGGING TESTS
// ============================================================================

TEST_F(LoggerCoverageTest, EnableFileLogging) {
  Logger::instance().set_file_output(test_log_file_.string());
  UNILINK_LOG_INFO("test", "file_log", "test message");
  Logger::instance().flush();

  std::this_thread::sleep_for(100ms);

  std::ifstream file(test_log_file_);
  EXPECT_TRUE(file.good());

  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  EXPECT_FALSE(content.empty());
}

TEST_F(LoggerCoverageTest, DisableFileLogging) {
  Logger::instance().set_file_output(test_log_file_.string());
  Logger::instance().set_file_output("");

  UNILINK_LOG_INFO("test", "disabled", "should not be in file");
  Logger::instance().flush();

  std::this_thread::sleep_for(100ms);
}

// ============================================================================
// CONSOLE LOGGING TESTS
// ============================================================================

TEST_F(LoggerCoverageTest, EnableDisableConsoleLogging) {
  Logger::instance().set_console_output(true);
  UNILINK_LOG_INFO("test", "console", "console message");

  Logger::instance().set_console_output(false);
  UNILINK_LOG_INFO("test", "console", "should not appear");

  Logger::instance().set_console_output(true);  // Re-enable for other tests
}

// ============================================================================
// ASYNC LOGGING TESTS
// ============================================================================

TEST_F(LoggerCoverageTest, EnableDisableAsyncLogging) {
  Logger::instance().set_async_logging(true);

  for (int i = 0; i < 10; i++) {
    UNILINK_LOG_INFO("test", "async", "async message " + std::to_string(i));
  }

  Logger::instance().flush();
  std::this_thread::sleep_for(100ms);

  Logger::instance().set_async_logging(false);
}

TEST_F(LoggerCoverageTest, FlushLogs) {
  Logger::instance().set_file_output(test_log_file_.string());
  Logger::instance().set_async_logging(true);

  UNILINK_LOG_INFO("test", "flush", "message before flush");
  Logger::instance().flush();

  std::this_thread::sleep_for(100ms);

  std::ifstream file(test_log_file_);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  EXPECT_TRUE(content.find("message before flush") != std::string::npos);

  Logger::instance().set_async_logging(false);
  Logger::instance().set_file_output("");
}

// ============================================================================
// LOG ROTATION TESTS
// ============================================================================

TEST_F(LoggerCoverageTest, EnableLogRotation) {
  LogRotationConfig config;
  config.max_file_size_bytes = 1024;  // 1KB
  config.max_files = 3;

  Logger::instance().set_file_output_with_rotation(test_log_file_.string(), config);

  // Write enough data to trigger rotation
  for (int i = 0; i < 100; i++) {
    UNILINK_LOG_INFO("test", "rotation", "This is a log message for rotation testing " + std::to_string(i));
  }

  Logger::instance().flush();
  std::this_thread::sleep_for(200ms);

  Logger::instance().set_file_output("");
}

TEST_F(LoggerCoverageTest, DisableLogRotation) {
  // Just test normal file logging without rotation
  Logger::instance().set_file_output(test_log_file_.string());
  UNILINK_LOG_INFO("test", "no_rotation", "message without rotation");
  Logger::instance().flush();
  Logger::instance().set_file_output("");
}

// ============================================================================
// LOG MACROS TESTS
// ============================================================================

TEST_F(LoggerCoverageTest, AllLogMacros) {
  Logger::instance().set_level(LogLevel::DEBUG);
  Logger::instance().set_console_output(true);

  UNILINK_LOG_DEBUG("component", "operation", "debug log");
  UNILINK_LOG_INFO("component", "operation", "info log");
  UNILINK_LOG_WARNING("component", "operation", "warning log");
  UNILINK_LOG_ERROR("component", "operation", "error log");

  Logger::instance().flush();
}

TEST_F(LoggerCoverageTest, LogWithDifferentComponents) {
  Logger::instance().set_level(LogLevel::INFO);

  UNILINK_LOG_INFO("tcp_server", "start", "starting server");
  UNILINK_LOG_INFO("tcp_client", "connect", "connecting to server");
  UNILINK_LOG_INFO("serial", "open", "opening port");
  UNILINK_LOG_INFO("memory_pool", "allocate", "allocating memory");

  Logger::instance().flush();
}

// ============================================================================
// COMPLEX SCENARIOS
// ============================================================================

TEST_F(LoggerCoverageTest, CombinedFileAndConsole) {
  Logger::instance().set_file_output(test_log_file_.string());
  Logger::instance().set_console_output(true);
  Logger::instance().set_level(LogLevel::DEBUG);

  UNILINK_LOG_DEBUG("test", "combined", "debug message");
  UNILINK_LOG_INFO("test", "combined", "info message");
  UNILINK_LOG_WARNING("test", "combined", "warning message");
  UNILINK_LOG_ERROR("test", "combined", "error message");

  Logger::instance().flush();
  std::this_thread::sleep_for(100ms);

  std::ifstream file(test_log_file_);
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  EXPECT_FALSE(content.empty());

  Logger::instance().set_file_output("");
}

TEST_F(LoggerCoverageTest, AsyncWithRotation) {
  Logger::instance().set_async_logging(true);

  LogRotationConfig config;
  config.max_file_size_bytes = 512;
  config.max_files = 2;
  Logger::instance().set_file_output_with_rotation(test_log_file_.string(), config);

  for (int i = 0; i < 50; i++) {
    UNILINK_LOG_INFO("test", "async_rot", "Log message number " + std::to_string(i) + " with some extra content");
  }

  Logger::instance().flush();
  std::this_thread::sleep_for(300ms);

  Logger::instance().set_async_logging(false);
  Logger::instance().set_file_output("");
}

TEST_F(LoggerCoverageTest, MultipleEnableDisableCycles) {
  for (int i = 0; i < 3; i++) {
    Logger::instance().set_file_output(test_log_file_.string());
    UNILINK_LOG_INFO("test", "cycle", "cycle " + std::to_string(i));
    Logger::instance().flush();
    Logger::instance().set_file_output("");
    std::this_thread::sleep_for(50ms);
  }
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(LoggerCoverageTest, EmptyMessages) {
  UNILINK_LOG_INFO("", "", "");
  UNILINK_LOG_DEBUG("test", "empty", "");
  Logger::instance().flush();
}

TEST_F(LoggerCoverageTest, LongMessages) {
  std::string long_msg(1000, 'x');
  UNILINK_LOG_INFO("test", "long", long_msg);
  Logger::instance().flush();
}

TEST_F(LoggerCoverageTest, SpecialCharacters) {
  UNILINK_LOG_INFO("test", "special", "Special chars: !@#$%^&*()[]{}|\\/<>?");
  UNILINK_LOG_INFO("test", "unicode", "Unicode: 你好 мир 🎉");
  Logger::instance().flush();
}

TEST_F(LoggerCoverageTest, RapidLogging) {
  Logger::instance().set_async_logging(true);

  for (int i = 0; i < 1000; i++) {
    UNILINK_LOG_INFO("test", "rapid", "msg" + std::to_string(i));
  }

  Logger::instance().flush();
  std::this_thread::sleep_for(200ms);
  Logger::instance().set_async_logging(false);
}
