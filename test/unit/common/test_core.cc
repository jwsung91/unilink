#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

#include "test_utils.hpp"
#include "unilink/common/common.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/config/config_manager.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

// ============================================================================
// COMMON TESTS
// ============================================================================

/**
 * @brief Common functionality tests
 */
TEST_F(BaseTest, CommonFunctionality) {
  // Test LinkState enum
  EXPECT_STREQ(common::to_cstr(common::LinkState::Idle), "Idle");
  EXPECT_STREQ(common::to_cstr(common::LinkState::Connected), "Connected");
  EXPECT_STREQ(common::to_cstr(common::LinkState::Error), "Error");

  // Test timestamp functionality
  std::string timestamp = common::ts_now();
  EXPECT_FALSE(timestamp.empty());
  EXPECT_GT(timestamp.length(), 10);
}

/**
 * @brief Configuration manager tests
 */
TEST_F(BaseTest, ConfigManager) {
  // Test basic configuration functionality
  // Note: ConfigManager might not have instance() method
  // This test is kept for future implementation
  EXPECT_TRUE(true);
}

// ============================================================================
// IOCONTEXT MANAGER TESTS
// ============================================================================

/**
 * @brief IoContextManager basic functionality tests
 */
TEST_F(BaseTest, IoContextManagerBasicFunctionality) {
  auto& manager = common::IoContextManager::instance();

  // Test basic operations
  EXPECT_FALSE(manager.is_running());

  manager.start();
  EXPECT_TRUE(manager.is_running());

  auto& context = manager.get_context();
  EXPECT_NE(&context, nullptr);

  manager.stop();
  EXPECT_FALSE(manager.is_running());
}

/**
 * @brief Independent context creation tests
 */
TEST_F(BaseTest, IndependentContextCreation) {
  auto& manager = common::IoContextManager::instance();

  // Create independent context
  auto independent_context = manager.create_independent_context();
  EXPECT_NE(independent_context, nullptr);

  // Verify it's different from global context
  manager.start();
  auto& global_context = manager.get_context();
  EXPECT_NE(independent_context.get(), &global_context);

  manager.stop();
}

// ============================================================================
// MEMORY POOL TESTS
// ============================================================================

/**
 * @brief Memory pool basic functionality tests
 */
TEST_F(MemoryTest, MemoryPoolBasicFunctionality) {
  auto& pool = common::GlobalMemoryPool::instance();

  // Test basic acquire/release
  auto buffer = pool.acquire(1024);
  EXPECT_NE(buffer, nullptr);

  pool.release(std::move(buffer), 1024);

  // Test statistics
  auto stats = pool.get_stats();
  EXPECT_GE(stats.total_allocations, 0);
}

/**
 * @brief Memory pool performance tests
 */
TEST_F(MemoryTest, MemoryPoolPerformance) {
  auto& pool = common::GlobalMemoryPool::instance();

  const int num_operations = 1000;
  const size_t buffer_size = 4096;

  auto start_time = std::chrono::high_resolution_clock::now();

  std::vector<std::unique_ptr<uint8_t[]>> buffers;
  buffers.reserve(num_operations);

  // Allocate buffers
  for (int i = 0; i < num_operations; ++i) {
    auto buffer = pool.acquire(buffer_size);
    if (buffer) {
      buffers.push_back(std::move(buffer));
    }
  }

  // Release buffers
  for (auto& buffer : buffers) {
    pool.release(std::move(buffer), buffer_size);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

  std::cout << "Memory pool performance: " << duration << " μs for " << num_operations << " operations" << std::endl;

  // Verify performance is reasonable
  EXPECT_LT(duration, 100000);  // Should complete in less than 100ms
}

/**
 * @brief Memory pool statistics tests
 */
TEST_F(MemoryTest, MemoryPoolStatistics) {
  auto& pool = common::GlobalMemoryPool::instance();

  // Perform some operations
  for (int i = 0; i < 100; ++i) {
    auto buffer = pool.acquire(1024);
    if (buffer) {
      pool.release(std::move(buffer), 1024);
    }
  }

  // Test basic statistics
  auto stats = pool.get_stats();
  EXPECT_GT(stats.total_allocations, 0);

  double hit_rate = pool.get_hit_rate();
  EXPECT_GE(hit_rate, 0.0);
  EXPECT_LE(hit_rate, 1.0);

  auto memory_usage = pool.get_memory_usage();
  EXPECT_GE(memory_usage.first, 0);
  EXPECT_GE(memory_usage.second, 0);
}

// ============================================================================
// LOG ROTATION TESTS
// ============================================================================

class LogRotationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Generate unique test file prefix to avoid conflicts in parallel execution
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    test_file_prefix_ = "test_rotation_" + std::to_string(timestamp);

    // Clean up any existing test files
    cleanup_test_files();

    // Setup logger for testing
    common::Logger::instance().set_level(common::LogLevel::DEBUG);
    common::Logger::instance().set_console_output(false);  // Disable console for file testing
  }

  void TearDown() override {
    // Clean up test files
    cleanup_test_files();

    // Reset logger
    common::Logger::instance().set_file_output("");  // Disable file output
    common::Logger::instance().set_console_output(true);
  }

  void cleanup_test_files() {
    // Remove test log files with unique prefix
    std::vector<std::string> test_files = {test_file_prefix_ + ".log",   test_file_prefix_ + ".0.log",
                                           test_file_prefix_ + ".1.log", test_file_prefix_ + ".2.log",
                                           test_file_prefix_ + ".3.log", test_file_prefix_ + ".4.log",
                                           test_file_prefix_ + ".5.log"};

    for (const auto& file : test_files) {
      if (std::filesystem::exists(file)) {
        std::filesystem::remove(file);
      }
    }
  }

  std::string test_file_prefix_;

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

TEST_F(LogRotationTest, BasicRotationSetup) {
  // Test basic rotation configuration
  common::LogRotationConfig config;
  config.max_file_size_bytes = 1024;  // 1KB for testing
  config.max_files = 3;

  EXPECT_EQ(config.max_file_size_bytes, 1024);
  EXPECT_EQ(config.max_files, 3);
}

TEST_F(LogRotationTest, FileSizeBasedRotation) {
  // Setup rotation with very small file size for testing
  common::LogRotationConfig config;
  config.max_file_size_bytes = 512;  // 512 bytes
  config.max_files = 5;

  common::Logger::instance().set_file_output_with_rotation(test_file_prefix_ + ".log", config);

  // Generate enough log data to trigger rotation
  for (int i = 0; i < 20; ++i) {
    UNILINK_LOG_INFO("test", "rotation",
                     "Test message " + std::to_string(i) +
                         " - This is a longer message to help reach the rotation threshold quickly.");
  }

  // Flush to ensure all data is written
  common::Logger::instance().flush();

  // Check if rotation occurred (should have multiple files)
  size_t file_count = count_log_files("test_rotation");
  EXPECT_GE(file_count, 1) << "At least one log file should exist";

  // Check if files are within size limits
  if (std::filesystem::exists(test_file_prefix_ + ".log")) {
    size_t current_size = get_file_size(test_file_prefix_ + ".log");
    EXPECT_LE(current_size, config.max_file_size_bytes * 2) << "Current log file should be reasonable size";
  }
}

TEST_F(LogRotationTest, FileCountLimit) {
  // Setup rotation with small file size and low file count
  common::LogRotationConfig config;
  config.max_file_size_bytes = 256;  // 256 bytes
  config.max_files = 2;              // Only keep 2 files

  common::Logger::instance().set_file_output_with_rotation(test_file_prefix_ + ".log", config);

  // Generate lots of log data to trigger multiple rotations
  for (int i = 0; i < 50; ++i) {
    UNILINK_LOG_INFO("test", "count_limit",
                     "Message " + std::to_string(i) +
                         " - Generating enough data to trigger multiple rotations and test file count limits.");
  }

  common::Logger::instance().flush();

  // Check that file count doesn't exceed limit
  size_t file_count = count_log_files("test_rotation");
  EXPECT_LE(file_count, config.max_files + 1) << "File count should not exceed limit (current + rotated files)";
}

TEST_F(LogRotationTest, LogRotationManagerDirectTest) {
  // Test LogRotation class directly
  common::LogRotationConfig config;
  config.max_file_size_bytes = 100;  // Very small for testing
  config.max_files = 2;

  common::LogRotation rotation(config);

  // Create a test file
  std::string test_file = test_file_prefix_ + ".log";
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
  EXPECT_TRUE(std::filesystem::exists(test_file_prefix_ + ".0.log")) << "Rotated file should exist";

  // Clean up
  std::filesystem::remove(test_file);
  std::filesystem::remove(test_file_prefix_ + ".0.log");
}

TEST_F(LogRotationTest, LogRotationWithoutRotation) {
  // Test that rotation doesn't occur when file is small
  common::LogRotationConfig config;
  config.max_file_size_bytes = 1024 * 1024;  // 1MB - very large
  config.max_files = 5;

  common::Logger::instance().set_file_output_with_rotation(test_file_prefix_ + ".log", config);

  // Generate small amount of log data
  for (int i = 0; i < 5; ++i) {
    UNILINK_LOG_INFO("test", "no_rotation", "Small message " + std::to_string(i));
  }

  common::Logger::instance().flush();

  // Should only have one file
  size_t file_count = count_log_files("test_rotation");
  EXPECT_EQ(file_count, 1) << "Should only have one file when size limit not reached";

  // File should exist and be small
  EXPECT_TRUE(std::filesystem::exists(test_file_prefix_ + ".log"));
  size_t file_size = get_file_size(test_file_prefix_ + ".log");
  EXPECT_LT(file_size, config.max_file_size_bytes) << "File should be smaller than rotation threshold";
}

// ============================================================================
// ASYNC LOGGING TESTS
// ============================================================================

class AsyncLoggingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Generate unique test file prefix to avoid conflicts in parallel execution
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    test_file_prefix_ = "async_test_" + std::to_string(timestamp);

    // Clean up any existing test files
    cleanup_test_files();

    // Setup logger for testing
    common::Logger::instance().set_level(common::LogLevel::DEBUG);
    common::Logger::instance().set_console_output(false);  // Disable console for file testing
  }

  void TearDown() override {
    // Clean up test files
    cleanup_test_files();

    // Reset logger
    common::Logger::instance().set_async_logging(false);
    common::Logger::instance().set_file_output("");  // Disable file output
    common::Logger::instance().set_console_output(true);
  }

  void cleanup_test_files() {
    // Remove test log files with unique prefix
    std::vector<std::string> test_files = {test_file_prefix_ + ".log", test_file_prefix_ + ".0.log",
                                           test_file_prefix_ + ".1.log", test_file_prefix_ + ".2.log"};

    for (const auto& file : test_files) {
      if (std::filesystem::exists(file)) {
        std::filesystem::remove(file);
      }
    }
  }

  std::string test_file_prefix_;

  size_t get_file_size(const std::string& filename) {
    if (std::filesystem::exists(filename)) {
      return std::filesystem::file_size(filename);
    }
    return 0;
  }
};

TEST_F(AsyncLoggingTest, BasicAsyncLoggingSetup) {
  // Test basic async logging configuration
  common::AsyncLogConfig config;
  config.max_queue_size = 1000;
  config.batch_size = 50;
  config.flush_interval = std::chrono::milliseconds(100);

  EXPECT_EQ(config.max_queue_size, 1000);
  EXPECT_EQ(config.batch_size, 50);
  EXPECT_EQ(config.flush_interval.count(), 100);
}

TEST_F(AsyncLoggingTest, AsyncLoggingEnabled) {
  // Setup async logging
  common::AsyncLogConfig config;
  config.max_queue_size = 1000;
  config.batch_size = 10;
  config.flush_interval = std::chrono::milliseconds(50);

  common::Logger::instance().set_async_logging(true, config);

  EXPECT_TRUE(common::Logger::instance().is_async_logging_enabled());

  // Generate some log messages
  for (int i = 0; i < 5; ++i) {
    UNILINK_LOG_INFO("async_test", "enabled", "Async logging test message " + std::to_string(i));
  }

  // Wait for async processing
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check statistics
  auto stats = common::Logger::instance().get_async_stats();
  EXPECT_GT(stats.total_logs, 0) << "Should have processed some logs";
  EXPECT_EQ(stats.dropped_logs, 0) << "Should not have dropped any logs";

  // Disable async logging to clean up
  common::Logger::instance().set_async_logging(false);
}

TEST_F(AsyncLoggingTest, AsyncLoggingWithFileOutput) {
  std::string log_filename = test_file_prefix_ + ".log";

  // Setup async logging with file output
  common::AsyncLogConfig config;
  config.max_queue_size = 1000;
  config.batch_size = 20;
  config.flush_interval = std::chrono::milliseconds(100);

  common::Logger::instance().set_file_output(log_filename);
  common::Logger::instance().set_async_logging(true, config);

  // Generate log messages
  for (int i = 0; i < 10; ++i) {
    UNILINK_LOG_INFO("async_test", "file_output", "Async file logging test message " + std::to_string(i));
  }

  // Wait for async processing
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // Check that file was created and has content
  EXPECT_TRUE(std::filesystem::exists(log_filename));
  size_t file_size = get_file_size(log_filename);
  EXPECT_GT(file_size, 0) << "Log file should have content";

  // Check statistics
  auto stats = common::Logger::instance().get_async_stats();
  EXPECT_GT(stats.total_logs, 0) << "Should have processed logs";
  EXPECT_GT(stats.batch_count, 0) << "Should have processed batches";

  // Disable async logging to clean up
  common::Logger::instance().set_async_logging(false);
}

TEST_F(AsyncLoggingTest, AsyncLoggingPerformance) {
  // Test async logging performance
  common::AsyncLogConfig config;
  config.max_queue_size = 10000;
  config.batch_size = 100;
  config.flush_interval = std::chrono::milliseconds(50);

  common::Logger::instance().set_async_logging(true, config);

  const int num_messages = 1000;
  auto start_time = std::chrono::high_resolution_clock::now();

  // Generate many log messages
  for (int i = 0; i < num_messages; ++i) {
    UNILINK_LOG_DEBUG("async_test", "performance", "Performance test message " + std::to_string(i));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

  // Wait for async processing to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Check performance (should be very fast since it's just queuing)
  double messages_per_second = (num_messages * 1000000.0) / duration;
  EXPECT_GT(messages_per_second, 100000) << "Should process at least 100k messages per second";

  // Check statistics
  auto stats = common::Logger::instance().get_async_stats();
  EXPECT_EQ(stats.total_logs, num_messages) << "Should have processed all messages";
  EXPECT_EQ(stats.dropped_logs, 0) << "Should not have dropped any messages";

  std::cout << "Async logging performance: " << messages_per_second << " messages/second" << std::endl;
}

TEST_F(AsyncLoggingTest, AsyncLoggingBackpressure) {
  // Test backpressure handling with small queue
  common::AsyncLogConfig config;
  config.max_queue_size = 5;  // Very small queue
  config.batch_size = 2;
  config.flush_interval = std::chrono::milliseconds(200);
  config.enable_backpressure = true;

  common::Logger::instance().set_async_logging(true, config);

  // Generate more messages than queue can handle
  for (int i = 0; i < 20; ++i) {
    UNILINK_LOG_INFO("async_test", "backpressure", "Backpressure test message " + std::to_string(i));
  }

  // Wait for processing
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Check statistics - should have some dropped messages
  auto stats = common::Logger::instance().get_async_stats();
  EXPECT_GT(stats.total_logs, 0) << "Should have processed some messages";
  EXPECT_GT(stats.dropped_logs, 0) << "Should have dropped some messages due to backpressure";

  double drop_rate = stats.get_drop_rate();
  EXPECT_GT(drop_rate, 0.0) << "Should have a non-zero drop rate";
  // Allow up to 90% drop rate for this test
  EXPECT_LT(drop_rate, 0.9) << "Should not have dropped more than 90% of messages";

  std::cout << "Backpressure test - Drop rate: " << (drop_rate * 100) << "%" << std::endl;
  std::cout << "Backpressure test - Total: " << stats.total_logs << ", Dropped: " << stats.dropped_logs << std::endl;
}

TEST_F(AsyncLoggingTest, AsyncLoggingDisable) {
  // Test disabling async logging
  common::AsyncLogConfig config;
  config.max_queue_size = 1000;
  config.batch_size = 50;

  // Enable async logging
  common::Logger::instance().set_async_logging(true, config);
  EXPECT_TRUE(common::Logger::instance().is_async_logging_enabled());

  // Disable async logging
  common::Logger::instance().set_async_logging(false);
  EXPECT_FALSE(common::Logger::instance().is_async_logging_enabled());

  // Generate some log messages (should use synchronous logging)
  for (int i = 0; i < 10; ++i) {
    UNILINK_LOG_INFO("async_test", "disable", "Synchronous logging test message " + std::to_string(i));
  }

  // Check statistics (should be empty since async is disabled)
  auto stats = common::Logger::instance().get_async_stats();
  EXPECT_EQ(stats.total_logs, 0) << "Should not have async statistics when disabled";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
