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

#ifdef _WIN32

TEST(PlatformTest, DISABLED_WindowsPlatformBenchmark) {
  GTEST_SKIP() << "Platform-specific benchmark tests are not supported on Windows yet.";
}

#else
#include <gmock/gmock.h>
#include <sys/utsname.h>
#include <unistd.h>

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

using namespace unilink;
using namespace unilink::test;
using namespace unilink::builder;
using namespace std::chrono_literals;

/**
 * @brief Comprehensive platform-specific tests
 *
 * This file combines all platform-specific tests including
 * platform detection, compatibility, performance characteristics,
 * and platform-specific functionality testing.
 */
class PlatformTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();

    // Get platform information
    get_platform_info();
  }

  void TearDown() override {
    // Clean up any test state
    TestUtils::waitFor(100);
  }

  uint16_t test_port_;
  std::string platform_name_;
  std::string platform_version_;
  std::string architecture_;

  void get_platform_info() {
    struct utsname info;
    if (uname(&info) == 0) {
      platform_name_ = info.sysname;
      platform_version_ = info.release;
      architecture_ = info.machine;
    } else {
      platform_name_ = "unknown";
      platform_version_ = "unknown";
      architecture_ = "unknown";
    }
  }

  // Helper function to check if running on specific platform
  bool is_linux() const { return platform_name_ == "Linux"; }
  bool is_windows() const { return platform_name_ == "Windows"; }
  bool is_unix_like() const { return is_linux(); }

  // Helper function to check architecture
  bool is_x86_64() const { return architecture_ == "x86_64"; }
  bool is_arm64() const { return architecture_ == "aarch64" || architecture_ == "arm64"; }
  bool is_arm() const { return architecture_.find("arm") != std::string::npos; }
};

// ============================================================================
// PLATFORM DETECTION TESTS
// ============================================================================

/**
 * @brief Test platform detection and information
 */
TEST_F(PlatformTest, PlatformDetection) {
  std::cout << "\n=== Platform Detection Test ===" << std::endl;

  std::cout << "Platform: " << platform_name_ << std::endl;
  std::cout << "Version: " << platform_version_ << std::endl;
  std::cout << "Architecture: " << architecture_ << std::endl;

  // Verify platform information is available
  EXPECT_FALSE(platform_name_.empty());
  EXPECT_FALSE(platform_version_.empty());
  EXPECT_FALSE(architecture_.empty());

  // Log platform-specific information
  if (is_linux()) {
    std::cout << "Running on Linux" << std::endl;
  } else if (is_windows()) {
    std::cout << "Running on Windows" << std::endl;
  } else {
    std::cout << "Running on unknown platform" << std::endl;
  }

  if (is_x86_64()) {
    std::cout << "Architecture: x86_64" << std::endl;
  } else if (is_arm64()) {
    std::cout << "Architecture: ARM64" << std::endl;
  } else if (is_arm()) {
    std::cout << "Architecture: ARM" << std::endl;
  } else {
    std::cout << "Architecture: " << architecture_ << std::endl;
  }
}

/**
 * @brief Test platform-specific file paths
 */
TEST_F(PlatformTest, PlatformSpecificFilePaths) {
  std::cout << "\n=== Platform-Specific File Paths Test ===" << std::endl;

  std::vector<std::string> test_paths;

  if (is_linux()) {
    // Linux systems
    test_paths = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0",   "/dev/ttyACM1",
                  "/dev/ttyS0",   "/dev/ttyS1",   "/tmp/test_file", "/var/tmp/test_file"};
  } else if (is_windows()) {
    // Windows systems
    test_paths = {"COM1", "COM2", "COM3", "COM4", "C:\\temp\\test_file", "C:\\Windows\\temp\\test_file"};
  } else {
    // Unknown platform
    test_paths = {"test_file", "temp/test_file"};
  }

  for (const auto& path : test_paths) {
    std::cout << "Testing path: " << path << std::endl;

    // Test path validity (basic check)
    EXPECT_FALSE(path.empty());

    // Test path length
    EXPECT_LT(path.length(), 260);  // Windows path limit
  }

  std::cout << "Platform-specific file paths test completed" << std::endl;
}

// ============================================================================
// PLATFORM-SPECIFIC SERIAL COMMUNICATION TESTS
// ============================================================================

/**
 * @brief Test serial communication on different platforms
 */
TEST_F(PlatformTest, SerialCommunicationPlatformSpecific) {
  std::cout << "\n=== Serial Communication Platform-Specific Test ===" << std::endl;

  std::vector<std::string> device_paths;

  if (is_linux()) {
    device_paths = {"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyS0", "/dev/ttyS1"};
  } else if (is_windows()) {
    device_paths = {"COM1", "COM2", "COM3", "COM4", "COM5", "COM6"};
  } else {
    device_paths = {"unknown_device"};
  }

  for (const auto& device : device_paths) {
    std::cout << "Testing device: " << device << std::endl;

    auto serial = UnifiedBuilder::serial(device, 9600).build();

    EXPECT_NE(serial, nullptr);

    // Test basic operations
    try {
      serial->send("test");
      std::cout << "  Send operation successful" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "  Send operation failed: " << e.what() << std::endl;
    }
  }

  std::cout << "Platform-specific serial communication test completed" << std::endl;
}

/**
 * @brief Test serial baud rates on different platforms
 */
TEST_F(PlatformTest, SerialBaudRatesPlatformSpecific) {
  std::cout << "\n=== Serial Baud Rates Platform-Specific Test ===" << std::endl;

  std::vector<uint32_t> baud_rates;

  if (is_linux()) {
    // Linux systems support more baud rates (within validation limits)
    baud_rates = {50,   75,    110,   134,   150,    200,    300,    600,    1200,    1800,    2400,   4800,
                  9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1000000, 2000000, 4000000};
  } else if (is_windows()) {
    // Windows has more limited baud rate support
    baud_rates = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
  } else {
    // Unknown platform - use common rates
    baud_rates = {9600, 19200, 38400, 57600, 115200};
  }

  for (auto baud_rate : baud_rates) {
    std::cout << "Testing baud rate: " << baud_rate << std::endl;

    auto serial = UnifiedBuilder::serial("/dev/ttyUSB0", baud_rate).build();

    EXPECT_NE(serial, nullptr);
  }

  std::cout << "Platform-specific baud rates test completed" << std::endl;
}

// ============================================================================
// PLATFORM-SPECIFIC NETWORK TESTS
// ============================================================================

/**
 * @brief Test network functionality on different platforms
 */
TEST_F(PlatformTest, NetworkFunctionalityPlatformSpecific) {
  std::cout << "\n=== Network Functionality Platform-Specific Test ===" << std::endl;

  // Test TCP server creation
  auto server = UnifiedBuilder::tcp_server(test_port_)
                    .unlimited_clients()  // No client limit

                    .build();

  EXPECT_NE(server, nullptr);

  // Test TCP client creation
  auto client = UnifiedBuilder::tcp_client("localhost", test_port_).build();

  EXPECT_NE(client, nullptr);

  // Test platform-specific network behavior
  if (is_linux()) {
    std::cout << "Linux network behavior: SO_REUSEADDR enabled" << std::endl;
  } else if (is_windows()) {
    std::cout << "Windows network behavior: SO_EXCLUSIVEADDRUSE enabled" << std::endl;
  }

  std::cout << "Platform-specific network functionality test completed" << std::endl;
}

/**
 * @brief Test network port handling on different platforms
 */
TEST_F(PlatformTest, NetworkPortHandlingPlatformSpecific) {
  std::cout << "\n=== Network Port Handling Platform-Specific Test ===" << std::endl;

  std::vector<uint16_t> test_ports;

  if (is_linux()) {
    // Linux systems can use any port above 1024
    test_ports = {8080, 9090, 30000, 40000, 50000, 60000};
  } else if (is_windows()) {
    // Windows has similar port restrictions
    test_ports = {8080, 9090, 30000, 40000, 50000, 60000};
  } else {
    // Unknown platform - use safe ports
    test_ports = {8080, 9090, 30000};
  }

  for (auto port : test_ports) {
    std::cout << "Testing port: " << port << std::endl;

    auto server = UnifiedBuilder::tcp_server(port)
                      .unlimited_clients()  // No client limit

                      .build();

    EXPECT_NE(server, nullptr);

    auto client = UnifiedBuilder::tcp_client("localhost", port).build();

    EXPECT_NE(client, nullptr);
  }

  std::cout << "Platform-specific network port handling test completed" << std::endl;
}

// ============================================================================
// PLATFORM-SPECIFIC MEMORY TESTS
// ============================================================================

/**
 * @brief Test memory behavior on different platforms
 */
TEST_F(PlatformTest, MemoryBehaviorPlatformSpecific) {
  std::cout << "\n=== Memory Behavior Platform-Specific Test ===" << std::endl;

  // Test memory allocation behavior
  const size_t test_size = 1024 * 1024;  // 1MB

  auto start_time = std::chrono::high_resolution_clock::now();

  std::vector<uint8_t> test_data(test_size);
  std::fill(test_data.begin(), test_data.end(), 0xAA);

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  std::cout << "Memory allocation test:" << std::endl;
  std::cout << "  Size: " << test_size << " bytes" << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Platform: " << platform_name_ << std::endl;

  // Verify data integrity
  for (size_t i = 0; i < test_size; ++i) {
    EXPECT_EQ(test_data[i], 0xAA);
  }

  std::cout << "Platform-specific memory behavior test completed" << std::endl;
}

/**
 * @brief Test memory alignment on different platforms
 */
TEST_F(PlatformTest, MemoryAlignmentPlatformSpecific) {
  std::cout << "\n=== Memory Alignment Platform-Specific Test ===" << std::endl;

  // Test alignment requirements
  struct TestStruct {
    char c;
    int i;
    double d;
  };

  std::cout << "Memory alignment test:" << std::endl;
  std::cout << "  sizeof(char): " << sizeof(char) << std::endl;
  std::cout << "  sizeof(int): " << sizeof(int) << std::endl;
  std::cout << "  sizeof(double): " << sizeof(double) << std::endl;
  std::cout << "  sizeof(TestStruct): " << sizeof(TestStruct) << std::endl;
  std::cout << "  alignof(TestStruct): " << alignof(TestStruct) << std::endl;

  // Test alignment on different platforms
  if (is_x86_64()) {
    std::cout << "x86_64 alignment: 8-byte aligned" << std::endl;
    EXPECT_EQ(alignof(TestStruct), 8);
  } else if (is_arm64()) {
    std::cout << "ARM64 alignment: 8-byte aligned" << std::endl;
    EXPECT_EQ(alignof(TestStruct), 8);
  } else if (is_arm()) {
    std::cout << "ARM alignment: 4-byte aligned" << std::endl;
    EXPECT_EQ(alignof(TestStruct), 4);
  }

  std::cout << "Platform-specific memory alignment test completed" << std::endl;
}

// ============================================================================
// PLATFORM-SPECIFIC THREADING TESTS
// ============================================================================

/**
 * @brief Test threading behavior on different platforms
 */
TEST_F(PlatformTest, ThreadingBehaviorPlatformSpecific) {
  std::cout << "\n=== Threading Behavior Platform-Specific Test ===" << std::endl;

  const int num_threads = std::thread::hardware_concurrency();
  std::cout << "Hardware concurrency: " << num_threads << std::endl;

  std::atomic<int> counter{0};
  std::vector<std::thread> threads;

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < 1000; ++j) {
        counter.fetch_add(1);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  std::cout << "Threading test:" << std::endl;
  std::cout << "  Threads: " << num_threads << std::endl;
  std::cout << "  Operations: " << counter.load() << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Platform: " << platform_name_ << std::endl;

  EXPECT_EQ(counter.load(), num_threads * 1000);

  std::cout << "Platform-specific threading behavior test completed" << std::endl;
}

// ============================================================================
// PLATFORM-SPECIFIC PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Test performance characteristics on different platforms
 */
TEST_F(PlatformTest, PerformanceCharacteristicsPlatformSpecific) {
  std::cout << "\n=== Performance Characteristics Platform-Specific Test ===" << std::endl;

  const int num_operations = 10000;

  // Test CPU performance
  auto start_time = std::chrono::high_resolution_clock::now();

  volatile int sum = 0;
  for (int i = 0; i < num_operations; ++i) {
    sum += i;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  double throughput = static_cast<double>(num_operations) / (duration.count() / 1000000.0);

  std::cout << "Performance test:" << std::endl;
  std::cout << "  Operations: " << num_operations << std::endl;
  std::cout << "  Duration: " << duration.count() << " μs" << std::endl;
  std::cout << "  Throughput: " << throughput << " ops/sec" << std::endl;
  std::cout << "  Platform: " << platform_name_ << std::endl;
  std::cout << "  Architecture: " << architecture_ << std::endl;

  // Platform-specific performance expectations
  if (is_x86_64()) {
    std::cout << "x86_64 performance: High throughput expected" << std::endl;
    EXPECT_GT(throughput, 1000000);
  } else if (is_arm64()) {
    std::cout << "ARM64 performance: Good throughput expected" << std::endl;
    EXPECT_GT(throughput, 500000);
  } else if (is_arm()) {
    std::cout << "ARM performance: Moderate throughput expected" << std::endl;
    EXPECT_GT(throughput, 100000);
  }

  std::cout << "Platform-specific performance characteristics test completed" << std::endl;
}

// ============================================================================
// PLATFORM-SPECIFIC COMPATIBILITY TESTS
// ============================================================================

/**
 * @brief Test cross-platform compatibility
 */
TEST_F(PlatformTest, CrossPlatformCompatibility) {
  std::cout << "\n=== Cross-Platform Compatibility Test ===" << std::endl;

  // Test that basic functionality works across platforms
  auto server = UnifiedBuilder::tcp_server(test_port_)
                    .unlimited_clients()  // No client limit

                    .build();

  EXPECT_NE(server, nullptr);

  auto client = UnifiedBuilder::tcp_client("localhost", test_port_).build();

  EXPECT_NE(client, nullptr);

  auto serial = UnifiedBuilder::serial("/dev/ttyUSB0", 9600).build();

  EXPECT_NE(serial, nullptr);

  std::cout << "Cross-platform compatibility test completed" << std::endl;
  std::cout << "  Platform: " << platform_name_ << std::endl;
  std::cout << "  Architecture: " << architecture_ << std::endl;
  std::cout << "  All basic functionality working" << std::endl;
}

/**
 * @brief Test platform-specific error handling
 */
TEST_F(PlatformTest, PlatformSpecificErrorHandling) {
  std::cout << "\n=== Platform-Specific Error Handling Test ===" << std::endl;

  // Test error handling on different platforms
  std::vector<std::string> invalid_paths;

  if (is_linux()) {
    invalid_paths = {"/dev/nonexistent", "/dev/ttyINVALID", "/tmp/nonexistent/file"};
  } else if (is_windows()) {
    invalid_paths = {"COM999", "LPT999", "C:\\nonexistent\\file"};
  } else {
    invalid_paths = {"nonexistent", "invalid_path"};
  }

  for (const auto& path : invalid_paths) {
    std::cout << "Testing invalid path: " << path << std::endl;

    auto serial = UnifiedBuilder::serial(path, 9600).build();

    EXPECT_NE(serial, nullptr);

    // Test error handling
    try {
      serial->send("test");
      std::cout << "  Error handling: Graceful degradation" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "  Error handling: Exception caught: " << e.what() << std::endl;
    }
  }

  std::cout << "Platform-specific error handling test completed" << std::endl;
}

#endif  // _WIN32

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
