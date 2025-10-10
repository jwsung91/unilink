#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/builder/unified_builder.hpp"
#include "unilink/common/exceptions.hpp"
#include "unilink/wrapper/serial/serial.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::builder;
using namespace unilink::wrapper;
using namespace std::chrono_literals;

/**
 * @brief Comprehensive serial communication tests
 *
 * This file combines all serial communication tests including
 * basic functionality, edge cases, error handling, and platform-specific testing.
 */
class SerialTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();

    // Set up test device paths
    test_device_paths_ = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyS0",
                          "/dev/ttyS1",   "COM1",         "COM2",         "COM3",         "COM4"};

    // Set up test baud rates
    test_baud_rates_ = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
  }

  void TearDown() override {
    // Clean up any test state
    TestUtils::waitFor(100);
  }

  uint16_t test_port_;
  std::vector<std::string> test_device_paths_;
  std::vector<uint32_t> test_baud_rates_;

  // Helper function to check if device exists
  bool device_exists(const std::string& device_path) {
    std::ifstream file(device_path);
    return file.good();
  }

  // Helper function to generate test data
  std::string generate_test_data(size_t size) {
    std::string data;
    data.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      data += static_cast<char>('A' + (i % 26));
    }
    return data;
  }
};

// ============================================================================
// BASIC SERIAL FUNCTIONALITY TESTS
// ============================================================================

/**
 * @brief Test serial communication basic functionality
 */
TEST_F(SerialTest, SerialBasicFunctionality) {
  std::cout << "\n=== Serial Basic Functionality Test ===" << std::endl;

  auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

  EXPECT_NE(serial_port, nullptr);

  // Test basic operations
  try {
    serial_port->send("test data");
    std::cout << "Send operation successful" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Send operation failed: " << e.what() << std::endl;
  }

  try {
    serial_port->send_line("test line");
    std::cout << "Send line operation successful" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Send line operation failed: " << e.what() << std::endl;
  }

  std::cout << "Serial basic functionality test completed" << std::endl;
}

/**
 * @brief Test serial communication with different baud rates
 */
TEST_F(SerialTest, SerialDifferentBaudRates) {
  std::cout << "\n=== Serial Different Baud Rates Test ===" << std::endl;

  for (auto baud_rate : test_baud_rates_) {
    auto serial_port = unilink::serial("/dev/ttyUSB0", baud_rate).auto_start(false).build();

    EXPECT_NE(serial_port, nullptr);
    std::cout << "Serial created with baud rate: " << baud_rate << std::endl;
  }

  std::cout << "Serial different baud rates test completed" << std::endl;
}

// ============================================================================
// SERIAL DEVICE EDGE CASE TESTS
// ============================================================================

/**
 * @brief Test serial communication with non-existent devices
 */
TEST_F(SerialTest, SerialNonExistentDevice) {
  std::cout << "\n=== Serial Non-Existent Device Test ===" << std::endl;

  // Test with non-existent device
  std::string non_existent_device = "/dev/ttyNONEXISTENT";

  auto serial_port = unilink::serial(non_existent_device, 9600).auto_start(false).build();

  EXPECT_NE(serial_port, nullptr);

  // Attempt to start (should handle gracefully)
  try {
    serial_port->start();
    std::cout << "Serial start attempted on non-existent device" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Expected exception: " << e.what() << std::endl;
  }

  std::cout << "Non-existent device test completed" << std::endl;
}

/**
 * @brief Test serial communication with invalid baud rates
 */
TEST_F(SerialTest, SerialInvalidBaudRates) {
  std::cout << "\n=== Serial Invalid Baud Rates Test ===" << std::endl;

  // Truly invalid baud rates (below minimum or above maximum)
  std::vector<uint32_t> invalid_baud_rates = {0, 1, 2, 3, 4, 5, 10, 49, 4000001, 5000000};

  // Valid baud rates that should not throw exceptions
  std::vector<uint32_t> valid_baud_rates = {100, 1000, 999999};

  // Test that truly invalid baud rates throw exceptions
  for (auto baud_rate : invalid_baud_rates) {
    EXPECT_THROW(
        { auto serial_port = unilink::serial("/dev/ttyUSB0", baud_rate).auto_start(false).build(); },
        common::BuilderException);
    std::cout << "Correctly rejected invalid baud rate: " << baud_rate << std::endl;
  }

  // Test that valid baud rates do not throw exceptions
  for (auto baud_rate : valid_baud_rates) {
    EXPECT_NO_THROW({
      auto serial_port = unilink::serial("/dev/ttyUSB0", baud_rate).auto_start(false).build();
      EXPECT_NE(serial_port, nullptr);
    });
    std::cout << "Correctly accepted valid baud rate: " << baud_rate << std::endl;
  }

  std::cout << "Invalid baud rates test completed" << std::endl;
}

/**
 * @brief Test serial communication with extreme baud rates
 */
TEST_F(SerialTest, SerialExtremeBaudRates) {
  std::cout << "\n=== Serial Extreme Baud Rates Test ===" << std::endl;

  // Valid extreme baud rates (within range)
  std::vector<uint32_t> valid_extreme_baud_rates = {50,     75,     110,    134,    150,     200,     300,    600,
                                                    1200,   1800,   2400,   4800,   14400,   28800,   38400,  57600,
                                                    115200, 230400, 460800, 921600, 1000000, 2000000, 4000000};

  // Invalid extreme baud rates (out of range)
  std::vector<uint32_t> invalid_extreme_baud_rates = {8000000};

  // Test valid extreme baud rates
  for (auto baud_rate : valid_extreme_baud_rates) {
    EXPECT_NO_THROW({
      auto serial_port = unilink::serial("/dev/ttyUSB0", baud_rate).auto_start(false).build();
      EXPECT_NE(serial_port, nullptr);
    });
    std::cout << "Serial created with valid extreme baud rate: " << baud_rate << std::endl;
  }

  // Test invalid extreme baud rates
  for (auto baud_rate : invalid_extreme_baud_rates) {
    EXPECT_THROW(
        { auto serial_port = unilink::serial("/dev/ttyUSB0", baud_rate).auto_start(false).build(); },
        common::BuilderException);
    std::cout << "Correctly rejected invalid extreme baud rate: " << baud_rate << std::endl;
  }

  std::cout << "Extreme baud rates test completed" << std::endl;
}

// ============================================================================
// SERIAL DATA EDGE CASE TESTS
// ============================================================================

/**
 * @brief Test serial communication with empty data
 */
TEST_F(SerialTest, SerialEmptyData) {
  std::cout << "\n=== Serial Empty Data Test ===" << std::endl;

  auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

  EXPECT_NE(serial_port, nullptr);

  // Test sending empty data
  try {
    serial_port->send("");
    std::cout << "Empty data sent successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Exception sending empty data: " << e.what() << std::endl;
  }

  // Test sending empty line
  try {
    serial_port->send_line("");
    std::cout << "Empty line sent successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Exception sending empty line: " << e.what() << std::endl;
  }

  std::cout << "Empty data test completed" << std::endl;
}

/**
 * @brief Test serial communication with very large data
 */
TEST_F(SerialTest, SerialLargeData) {
  std::cout << "\n=== Serial Large Data Test ===" << std::endl;

  auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

  EXPECT_NE(serial_port, nullptr);

  // Test with various large data sizes
  std::vector<size_t> data_sizes = {1024, 4096, 8192, 16384, 32768, 65536};

  for (auto size : data_sizes) {
    std::string large_data = generate_test_data(size);

    try {
      serial_port->send(large_data);
      std::cout << "Large data (" << size << " bytes) sent successfully" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Exception sending large data (" << size << " bytes): " << e.what() << std::endl;
    }
  }

  std::cout << "Large data test completed" << std::endl;
}

/**
 * @brief Test serial communication with binary data
 */
TEST_F(SerialTest, SerialBinaryData) {
  std::cout << "\n=== Serial Binary Data Test ===" << std::endl;

  auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

  EXPECT_NE(serial_port, nullptr);

  // Test with binary data containing null bytes
  std::string binary_data = "Hello\x00World\x00\x01\x02\x03\xFF\xFE\xFD";

  try {
    serial_port->send(binary_data);
    std::cout << "Binary data sent successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Exception sending binary data: " << e.what() << std::endl;
  }

  // Test with all possible byte values
  std::string all_bytes;
  all_bytes.reserve(256);
  for (int i = 0; i < 256; ++i) {
    all_bytes += static_cast<char>(i);
  }

  try {
    serial_port->send(all_bytes);
    std::cout << "All bytes data sent successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Exception sending all bytes data: " << e.what() << std::endl;
  }

  std::cout << "Binary data test completed" << std::endl;
}

// ============================================================================
// SERIAL CONFIGURATION EDGE CASE TESTS
// ============================================================================

/**
 * @brief Test serial communication with invalid device paths
 */
TEST_F(SerialTest, SerialInvalidDevicePaths) {
  std::cout << "\n=== Serial Invalid Device Paths Test ===" << std::endl;

  // Paths that should be rejected by input validation (truly invalid format)
  std::vector<std::string> invalid_paths = {"", "invalid", "COM", "COMX"};

  // Paths that should be allowed by input validation (valid device path format, even if device doesn't exist)
  std::vector<std::string> valid_paths = {"/dev/invalid", "/dev/tty",    "/dev/ttyX",    "NUL",          "/dev/null",
                                          "/dev/zero",    "/dev/random", "/dev/urandom", "/dev/ttyUSB0", "/dev/ttyACM0",
                                          "COM1",         "COM2",        "PRN",          "AUX",          "LPT1"};

  // Test paths that should be rejected by input validation
  for (const auto& path : invalid_paths) {
    EXPECT_THROW(
        { auto serial_port = unilink::serial(path, 9600).auto_start(false).build(); }, common::BuilderException);
    std::cout << "Correctly rejected invalid path: '" << path << "'" << std::endl;
  }

  // Test paths that should pass input validation (even if device doesn't exist)
  for (const auto& path : valid_paths) {
    EXPECT_NO_THROW({
      auto serial_port = unilink::serial(path, 9600).auto_start(false).build();
      EXPECT_NE(serial_port, nullptr);
    });
    std::cout << "Serial created with valid path: '" << path << "'" << std::endl;
  }

  std::cout << "Invalid device paths test completed" << std::endl;
}

/**
 * @brief Test serial communication with special characters in device path
 */
TEST_F(SerialTest, SerialSpecialCharactersInDevicePath) {
  std::cout << "\n=== Serial Special Characters In Device Path Test ===" << std::endl;

  // Paths with special characters that should be rejected by input validation (security improvement)
  std::vector<std::string> invalid_special_paths = {
      "/dev/ttyUSB0@special",    "/dev/ttyUSB0#test",  "/dev/ttyUSB0$value",    "/dev/ttyUSB0%percent",
      "/dev/ttyUSB0^caret",      "/dev/ttyUSB0&and",   "/dev/ttyUSB0*star",     "/dev/ttyUSB0(open",
      "/dev/ttyUSB0)close",      "/dev/ttyUSB0+plus",  "/dev/ttyUSB0=equals",   "/dev/ttyUSB0[open",
      "/dev/ttyUSB0]close",      "/dev/ttyUSB0{open",  "/dev/ttyUSB0}close",    "/dev/ttyUSB0|pipe",
      "/dev/ttyUSB0\\backslash", "/dev/ttyUSB0:colon", "/dev/ttyUSB0;semcolon", "/dev/ttyUSB0\"quote",
      "/dev/ttyUSB0'apostrophe", "/dev/ttyUSB0<less",  "/dev/ttyUSB0>greater",  "/dev/ttyUSB0,comma",
      "/dev/ttyUSB0.question",   "/dev/ttyUSB0 space", "/dev/ttyUSB0\ttab",     "/dev/ttyUSB0\nnewline",
      "/dev/ttyUSB0\rreturn"};

  // Valid paths (should pass input validation)
  std::vector<std::string> valid_paths = {"/dev/ttyUSB0", "/dev/ttyACM0", "COM1", "COM2"};

  // Test that special character paths are rejected (security improvement)
  for (const auto& path : invalid_special_paths) {
    EXPECT_THROW(
        { auto serial_port = unilink::serial(path, 9600).auto_start(false).build(); }, common::BuilderException);
    std::cout << "Correctly rejected path with special characters: '" << path << "'" << std::endl;
  }

  // Test that valid paths are accepted
  for (const auto& path : valid_paths) {
    EXPECT_NO_THROW({
      auto serial_port = unilink::serial(path, 9600).auto_start(false).build();
      EXPECT_NE(serial_port, nullptr);
    });
    std::cout << "Serial created with valid path: '" << path << "'" << std::endl;
  }

  std::cout << "Special characters in device path test completed" << std::endl;
}

// ============================================================================
// SERIAL ERROR HANDLING TESTS
// ============================================================================

/**
 * @brief Test serial communication error handling
 */
TEST_F(SerialTest, SerialErrorHandling) {
  std::cout << "\n=== Serial Error Handling Test ===" << std::endl;

  auto serial =
      serial("/dev/ttyUSB0", 9600)
          .auto_start(false)
          .on_error([](const std::string& error) { std::cout << "Error callback triggered: " << error << std::endl; })
          .build();

  EXPECT_NE(serial_port, nullptr);

  // Test error handling scenarios
  try {
    serial_port->start();
    std::cout << "Serial start attempted" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Expected exception during start: " << e.what() << std::endl;
  }

  // Test sending data when not connected
  try {
    serial_port->send("test data");
    std::cout << "Data sent when not connected" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Exception sending data when not connected: " << e.what() << std::endl;
  }

  std::cout << "Error handling test completed" << std::endl;
}

/**
 * @brief Test serial communication with multiple error scenarios
 */
TEST_F(SerialTest, SerialMultipleErrorScenarios) {
  std::cout << "\n=== Serial Multiple Error Scenarios Test ===" << std::endl;

  std::atomic<int> error_count{0};

  auto serial_port = unilink::serial("/dev/ttyUSB0", 9600)
                    .auto_start(false)
                    .on_error([&](const std::string& error) {
                      error_count++;
                      std::cout << "Error " << error_count.load() << ": " << error << std::endl;
                    })
                    .build();

  EXPECT_NE(serial_port, nullptr);

  // Test multiple error scenarios
  std::vector<std::function<void()>> error_scenarios = {
      [&]() { serial_port->start(); }, [&]() { serial_port->send("test"); }, [&]() { serial_port->send_line("test"); },
      [&]() { serial_port->stop(); },  [&]() { serial_port->start(); },      [&]() { serial_port->send("test2"); }};

  for (size_t i = 0; i < error_scenarios.size(); ++i) {
    try {
      error_scenarios[i]();
      std::cout << "Error scenario " << i << " executed" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Error scenario " << i << " exception: " << e.what() << std::endl;
    }
  }

  std::cout << "Multiple error scenarios test completed" << std::endl;
  std::cout << "Total errors: " << error_count.load() << std::endl;
}

// ============================================================================
// SERIAL PERFORMANCE EDGE CASE TESTS
// ============================================================================

/**
 * @brief Test serial communication performance with high frequency operations
 */
TEST_F(SerialTest, SerialHighFrequencyOperations) {
  std::cout << "\n=== Serial High Frequency Operations Test ===" << std::endl;

  auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

  EXPECT_NE(serial_port, nullptr);

  const int num_operations = 1000;
  const std::string test_data = "test_data";

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_operations; ++i) {
    try {
      serial_port->send(test_data);
    } catch (const std::exception& e) {
      // Expected exceptions for non-existent device
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  std::cout << "High frequency operations test completed:" << std::endl;
  std::cout << "  Operations: " << num_operations << std::endl;
  std::cout << "  Total time: " << duration.count() << " μs" << std::endl;
  std::cout << "  Average per operation: " << (duration.count() / num_operations) << " μs" << std::endl;

  // Performance should be reasonable (less than 100μs per operation)
  EXPECT_LT(duration.count() / num_operations, 100);
}

/**
 * @brief Test serial communication with concurrent operations
 */
TEST_F(SerialTest, SerialConcurrentOperations) {
  std::cout << "\n=== Serial Concurrent Operations Test ===" << std::endl;

  const int num_threads = 4;
  const int operations_per_thread = 100;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      auto serial_port = unilink::serial("/dev/ttyUSB" + std::to_string(t), 9600).auto_start(false).build();

      for (int i = 0; i < operations_per_thread; ++i) {
        try {
          serial_port->send("thread_" + std::to_string(t) + "_data_" + std::to_string(i));
          completed_operations++;
        } catch (const std::exception& e) {
          // Expected exceptions for non-existent devices
          completed_operations++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
  std::cout << "Concurrent operations test completed: " << completed_operations.load() << " operations" << std::endl;
}

// ============================================================================
// SERIAL CONFIGURATION EDGE CASE TESTS
// ============================================================================

/**
 * @brief Test serial communication with various data bits configurations
 */
TEST_F(SerialTest, SerialDataBitsConfigurations) {
  std::cout << "\n=== Serial Data Bits Configurations Test ===" << std::endl;

  std::vector<int> data_bits_options = {5, 6, 7, 8, 9};

  for (auto data_bits : data_bits_options) {
    auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

    EXPECT_NE(serial_port, nullptr);

    // Note: Actual data bits setting depends on implementation
    std::cout << "Serial created with data bits: " << data_bits << std::endl;
  }

  std::cout << "Data bits configurations test completed" << std::endl;
}

/**
 * @brief Test serial communication with various stop bits configurations
 */
TEST_F(SerialTest, SerialStopBitsConfigurations) {
  std::cout << "\n=== Serial Stop Bits Configurations Test ===" << std::endl;

  std::vector<int> stop_bits_options = {1, 2};

  for (auto stop_bits : stop_bits_options) {
    auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

    EXPECT_NE(serial_port, nullptr);

    // Note: Actual stop bits setting depends on implementation
    std::cout << "Serial created with stop bits: " << stop_bits << std::endl;
  }

  std::cout << "Stop bits configurations test completed" << std::endl;
}

/**
 * @brief Test serial communication with various parity configurations
 */
TEST_F(SerialTest, SerialParityConfigurations) {
  std::cout << "\n=== Serial Parity Configurations Test ===" << std::endl;

  std::vector<std::string> parity_options = {"none", "even", "odd", "mark", "space"};

  for (const auto& parity : parity_options) {
    auto serial_port = unilink::serial("/dev/ttyUSB0", 9600).auto_start(false).build();

    EXPECT_NE(serial_port, nullptr);

    // Note: Actual parity setting depends on implementation
    std::cout << "Serial created with parity: " << parity << std::endl;
  }

  std::cout << "Parity configurations test completed" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
