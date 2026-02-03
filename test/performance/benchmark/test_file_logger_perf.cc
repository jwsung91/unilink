#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>
#include "unilink/diagnostics/logger.hpp"

using namespace unilink::diagnostics;

class FileLoggerPerfTest : public ::testing::Test {
 protected:
  std::string log_filename;

  void SetUp() override {
    log_filename = "perf_test_log.txt";
    // Reset logger state
    Logger::instance().set_level(LogLevel::INFO);
    Logger::instance().set_outputs(0); // Disable console

    // Clean up any existing file
    if (std::filesystem::exists(log_filename)) {
      std::filesystem::remove(log_filename);
    }

    Logger::instance().set_file_output(log_filename);
  }

  void TearDown() override {
    // Restore default state
    Logger::instance().set_file_output(""); // Close file
    Logger::instance().set_outputs(static_cast<int>(LogOutput::CONSOLE));

    // Clean up file
    if (std::filesystem::exists(log_filename)) {
      std::filesystem::remove(log_filename);
    }
  }
};

TEST_F(FileLoggerPerfTest, FileWritePerformance) {
  const int iterations = 100000;
  std::string message = "This is a performance test message for file logging.";

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    Logger::instance().info("PerfTest", "Write", message);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "File Logging (" << iterations << " iter): " << duration << " ms ("
            << (double)duration / iterations * 1000.0 << " μs/call)" << std::endl;
}
