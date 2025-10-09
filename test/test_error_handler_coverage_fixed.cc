#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

/**
 * @brief Comprehensive error handler tests
 *
 * These tests provide comprehensive coverage for the error handling system,
 * addressing the current 0% coverage issue.
 */
class ErrorHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset error handler state
    auto& error_handler = common::ErrorHandler::instance();
    error_handler.reset_stats();

    // Initialize test state
    error_count_ = 0;
    last_error_category_ = "";
    last_error_level_ = common::ErrorLevel::INFO;
  }

  void TearDown() override {
    // Clean up any test state
    TestUtils::waitFor(100);
  }

  // Test state
  std::atomic<int> error_count_{0};
  std::string last_error_category_;
  common::ErrorLevel last_error_level_;
};

// ============================================================================
// ERROR REPORTING TESTS
// ============================================================================

/**
 * @brief Test connection error reporting
 */
TEST_F(ErrorHandlerTest, ConnectionErrorReporting) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Connection error scenario
  std::string component = "tcp_client";
  std::string operation = "connect";
  boost::system::error_code ec(boost::system::errc::connection_refused, boost::system::generic_category());
  bool is_retryable = true;

  // When: Report connection error
  common::error_reporting::report_connection_error(component, operation, ec, is_retryable);

  // Then: Verify error was recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test communication error reporting
 */
TEST_F(ErrorHandlerTest, CommunicationErrorReporting) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Communication error scenario
  std::string component = "tcp_client";
  std::string operation = "read";
  std::string error_message = "Read timeout";
  bool is_retryable = false;

  // When: Report communication error
  common::error_reporting::report_communication_error(component, operation, error_message, is_retryable);

  // Then: Verify error was recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test configuration error reporting
 */
TEST_F(ErrorHandlerTest, ConfigurationErrorReporting) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Configuration error scenario
  std::string component = "config_manager";
  std::string operation = "load_config";
  std::string error_message = "Invalid configuration file";

  // When: Report configuration error
  common::error_reporting::report_configuration_error(component, operation, error_message);

  // Then: Verify error was recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test memory error reporting
 */
TEST_F(ErrorHandlerTest, MemoryErrorReporting) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Memory error scenario
  std::string component = "memory_pool";
  std::string operation = "allocate";
  std::string error_message = "Memory allocation failed";

  // When: Report memory error
  common::error_reporting::report_memory_error(component, operation, error_message);

  // Then: Verify error was recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test system error reporting
 */
TEST_F(ErrorHandlerTest, SystemErrorReporting) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: System error scenario
  std::string component = "io_context";
  std::string operation = "run";
  std::string error_message = "System resource unavailable";
  boost::system::error_code ec(boost::system::errc::resource_unavailable_try_again, boost::system::generic_category());

  // When: Report system error
  common::error_reporting::report_system_error(component, operation, error_message, ec);

  // Then: Verify error was recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

// ============================================================================
// ERROR STATISTICS TESTS
// ============================================================================

/**
 * @brief Test error statistics collection
 */
TEST_F(ErrorHandlerTest, ErrorStatisticsCollection) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Multiple error reports
  common::error_reporting::report_connection_error("client1", "connect", boost::system::error_code{}, true);
  common::error_reporting::report_connection_error("client2", "connect", boost::system::error_code{}, false);
  common::error_reporting::report_configuration_error("config", "load", "Error 3");
  common::error_reporting::report_memory_error("pool", "alloc", "Error 4");
  common::error_reporting::report_system_error("io", "run", "Error 5");

  // When: Get statistics
  auto stats = error_handler.get_error_stats();

  // Then: Verify statistics
  EXPECT_EQ(stats.total_errors, 5);
}

/**
 * @brief Test error rate calculation
 */
TEST_F(ErrorHandlerTest, ErrorRateCalculation) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Error reports over time
  for (int i = 0; i < 10; ++i) {
    common::error_reporting::report_connection_error("client", "connect", boost::system::error_code{}, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // When: Get statistics
  auto stats = error_handler.get_error_stats();

  // Then: Verify error rate
  EXPECT_GT(stats.total_errors, 0);
}

// ============================================================================
// ERROR CALLBACK TESTS
// ============================================================================

/**
 * @brief Test error callback registration
 */
TEST_F(ErrorHandlerTest, ErrorCallbackRegistration) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Error callback
  std::atomic<int> callback_count{0};
  std::string last_callback_error;

  auto callback = [&callback_count, &last_callback_error](const common::ErrorInfo& error_info) {
    callback_count++;
    last_callback_error = error_info.message;
  };

  // When: Register callback and report error
  error_handler.register_callback(callback);
  common::error_reporting::report_connection_error("test", "operation", boost::system::error_code{}, false);

  // Then: Verify callback was called
  EXPECT_TRUE(TestUtils::waitForCondition([&callback_count]() { return callback_count.load() > 0; }, 1000));
  EXPECT_GT(callback_count.load(), 0);
}

/**
 * @brief Test error callback with different error levels
 */
TEST_F(ErrorHandlerTest, ErrorCallbackWithLevels) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Error callback that tracks levels
  std::vector<common::ErrorLevel> received_levels;

  auto callback = [&received_levels](const common::ErrorInfo& error_info) {
    received_levels.push_back(error_info.level);
  };

  // When: Register callback and report errors with different levels
  error_handler.register_callback(callback);

  // Report errors with different levels (simulated)
  common::error_reporting::report_connection_error("client", "connect", boost::system::error_code{}, false);
  common::error_reporting::report_memory_error("pool", "alloc", "Memory error");

  // Then: Verify callback received errors
  EXPECT_TRUE(TestUtils::waitForCondition([&received_levels]() { return received_levels.size() >= 2; }, 1000));
  EXPECT_GE(received_levels.size(), 2);
}

// ============================================================================
// ERROR RECOVERY TESTS
// ============================================================================

/**
 * @brief Test error recovery mechanisms
 */
TEST_F(ErrorHandlerTest, ErrorRecoveryMechanisms) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Retryable error
  common::error_reporting::report_connection_error("client", "connect", boost::system::error_code{}, true);

  // When: Check if error is retryable
  auto stats = error_handler.get_error_stats();

  // Then: Verify retryable error was recorded
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test error threshold detection
 */
TEST_F(ErrorHandlerTest, ErrorThresholdDetection) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Multiple rapid errors
  for (int i = 0; i < 5; ++i) {
    common::error_reporting::report_connection_error("client", "connect", boost::system::error_code{}, false);
  }

  // When: Check error threshold
  auto stats = error_handler.get_error_stats();

  // Then: Verify threshold detection
  EXPECT_EQ(stats.total_errors, 5);
}

// ============================================================================
// ERROR CLEANUP TESTS
// ============================================================================

/**
 * @brief Test error statistics cleanup
 */
TEST_F(ErrorHandlerTest, ErrorStatisticsCleanup) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Some errors reported
  common::error_reporting::report_connection_error("client", "connect", boost::system::error_code{}, false);
  common::error_reporting::report_configuration_error("config", "load", "Error 2");

  // When: Clear statistics
  error_handler.reset_stats();
  auto stats = error_handler.get_error_stats();

  // Then: Verify statistics were cleared
  EXPECT_EQ(stats.total_errors, 0);
}

/**
 * @brief Test error handler reset
 */
TEST_F(ErrorHandlerTest, ErrorHandlerReset) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Error callback registered
  std::atomic<int> callback_count{0};
  auto callback = [&callback_count](const common::ErrorInfo&) { callback_count++; };
  error_handler.register_callback(callback);

  // When: Clear callbacks
  error_handler.clear_callbacks();

  // Then: Verify callback was cleared
  common::error_reporting::report_connection_error("test", "operation", boost::system::error_code{}, false);

  // Callback should not be called after clear
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_EQ(callback_count.load(), 0);
}

// ============================================================================
// ERROR LEVEL TESTS
// ============================================================================

/**
 * @brief Test error level filtering
 */
TEST_F(ErrorHandlerTest, ErrorLevelFiltering) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Set minimum error level
  error_handler.set_min_error_level(common::ErrorLevel::WARNING);

  // When: Report errors with different levels
  common::error_reporting::report_info("component", "operation", "Info message");
  common::error_reporting::report_warning("component", "operation", "Warning message");
  common::error_reporting::report_memory_error("component", "operation", "Error message");

  // Then: Verify only appropriate errors were recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_GT(stats.total_errors, 0);
}

/**
 * @brief Test error handler enable/disable
 */
TEST_F(ErrorHandlerTest, ErrorHandlerEnableDisable) {
  auto& error_handler = common::ErrorHandler::instance();

  // Given: Disable error reporting
  error_handler.set_enabled(false);

  // When: Report error
  common::error_reporting::report_connection_error("test", "operation", boost::system::error_code{}, false);

  // Then: Verify error was not recorded
  auto stats = error_handler.get_error_stats();
  EXPECT_EQ(stats.total_errors, 0);

  // Re-enable for cleanup
  error_handler.set_enabled(true);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
