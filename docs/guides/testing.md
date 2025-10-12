# Testing Guide

Complete guide for testing `unilink`, including running tests, CI/CD integration, and writing custom tests.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Running Tests](#running-tests)
3. [Test Categories](#test-categories)
4. [Memory Safety Validation](#memory-safety-validation)
5. [Continuous Integration](#continuous-integration)
6. [Writing Custom Tests](#writing-custom-tests)

---

## Quick Start

### Build and Run All Tests

```bash
# 1. Build with tests enabled
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j

# 2. Run all tests
cd build
ctest --output-on-failure

# 3. View results
# All tests should pass with detailed output
```

---

## Running Tests

### Run All Tests

```bash
cd build
ctest --output-on-failure
```

**Expected output:**
```
Test project /path/to/unilink/build
    Start 1: BaseTest.CommonFunctionality
1/X Test #1: BaseTest.CommonFunctionality ................   Passed    0.XX sec
    ...
100% tests passed, 0 tests failed out of X

Total Test time (real) = XX.XX sec
```

---

### Run Specific Test Categories

Individual test executables are available:

```bash
# Core functionality tests
./build/test/run_core_tests

# Integration tests (end-to-end)
./build/test/run_integration_tests

# Memory safety tests
./build/test/run_memory_safety_tests

# Thread safety and concurrency tests
./build/test/run_concurrency_safety_tests

# Performance benchmarks
./build/test/run_performance_tests

# Stress and stability tests
./build/test/run_stress_tests
```

---

### Run Tests with Verbose Output

```bash
# CTest verbose mode
ctest --output-on-failure --verbose

# Run specific test with GTest output
./build/test/run_core_tests --gtest_filter=TcpClientTest.*
```

---

### Run Tests in Parallel

```bash
# Run tests in parallel (faster)
ctest -j $(nproc)

# Limit parallel jobs
ctest -j 4
```

---

## Test Categories

### Core Tests

Basic functionality and API tests.

```bash
./build/test/run_core_tests
```

**Tests:**
- Builder API functionality
- TCP client/server basic operations
- Serial port configuration
- Error handling
- State management

**Coverage:** Fundamental library features

---

### Integration Tests

End-to-end communication tests with real networking.

```bash
./build/test/run_integration_tests
```

**Tests:**
- TCP client-server communication
- Serial loopback tests
- Multi-client scenarios
- Connection lifecycle
- Data transfer validation

**Coverage:** Real-world usage scenarios

---

### Memory Safety Tests

Memory tracking, leak detection, and bounds checking.

```bash
./build/test/run_memory_safety_tests
```

**Tests:**
- Memory leak detection
- Buffer bounds checking
- Safe data conversions
- Memory pool validation
- Allocation tracking

**Example output:**
```
[==========] Running 10 tests from 1 test suite.
[----------] 10 tests from MemorySafetyTest
[ RUN      ] MemorySafetyTest.MemoryTrackingBasicFunctionality
[       OK ] MemorySafetyTest.MemoryTrackingBasicFunctionality (0 ms)
[ RUN      ] MemorySafetyTest.MemoryLeakDetection
[       OK ] MemorySafetyTest.MemoryLeakDetection (0 ms)
...
[----------] 10 tests from MemorySafetyTest (1 ms total)
[  PASSED  ] 10 tests.
```

---

### Concurrency Safety Tests

Thread safety and concurrent access patterns.

```bash
./build/test/run_concurrency_safety_tests
```

**Tests:**
- Concurrent send operations
- Thread-safe state management
- Multiple threads calling APIs
- Race condition detection
- Lock contention scenarios

**Coverage:** Multi-threaded safety

---

### Performance Tests

Performance benchmarks and optimization validation.

```bash
./build/test/run_performance_tests
```

**Tests:**
- TCP throughput measurement
- Latency benchmarks
- Memory allocation overhead
- Connection establishment time
- Concurrent connection handling

**Example output:**
```
TCP Throughput: 950 Mb/s
TCP Latency (avg): 120 μs
Memory Pool Hit Rate: 98.5%
```

---

### Stress Tests

High-load and stability testing.

```bash
./build/test/run_stress_tests
```

**Tests:**
- Long-running connections (24+ hours)
- High connection rate
- Large data transfers
- Memory stress
- Connection churn

**Coverage:** Stability and reliability

---

## Memory Safety Validation

### Built-in Memory Tracking

Enable memory tracking for development:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_MEMORY_TRACKING=ON \
  -DBUILD_TESTING=ON

cmake --build build -j
cd build && ctest
```

**Features:**
- Tracks all allocations and deallocations
- Detects memory leaks
- Reports memory usage patterns
- Zero overhead in Release builds

---

### AddressSanitizer (ASan)

Detect memory errors at runtime:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUNILINK_ENABLE_SANITIZERS=ON \
  -DBUILD_TESTING=ON

cmake --build build -j
cd build && ctest --output-on-failure
```

**Detects:**
- ✅ Use-after-free
- ✅ Heap buffer overflow
- ✅ Stack buffer overflow
- ✅ Memory leaks
- ✅ Use-after-return

**Note:** Tests run slower (~2-3x) with sanitizers enabled.

---

### ThreadSanitizer (TSan)

Detect thread race conditions:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread" \
  -DBUILD_TESTING=ON

cmake --build build -j
cd build && ctest
```

**Detects:**
- Data races
- Deadlocks
- Thread leaks

---

### Valgrind

Advanced memory debugging:

```bash
# Build with debug symbols
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j

# Run tests under Valgrind
cd build
ctest -T memcheck

# Or run specific test
valgrind --leak-check=full --show-leak-kinds=all \
  ./test/run_memory_safety_tests
```

---

## Continuous Integration

### GitHub Actions Integration

All tests are automatically run on every commit and pull request through GitHub Actions.

**CI/CD Badges:**

[![CI/CD Pipeline](https://github.com/jwsung91/unilink/actions/workflows/ci.yml/badge.svg)](https://github.com/jwsung91/unilink/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/jwsung91/unilink/actions/workflows/coverage.yml/badge.svg)](https://github.com/jwsung91/unilink/actions/workflows/coverage.yml)

---

### CI/CD Build Matrix

Tests run across multiple configurations:

| Platform | Compiler | Build Type | Sanitizers | Test Status |
|----------|----------|------------|------------|-------------|
| Ubuntu 22.04 | GCC 11 | Debug | ✅ | ✅ Full Testing |
| Ubuntu 22.04 | GCC 11 | Release | ❌ | ✅ Full Testing |
| Ubuntu 22.04 | Clang 14 | Debug | ✅ | ✅ Full Testing |
| Ubuntu 22.04 | Clang 14 | Release | ❌ | ✅ Full Testing |
| Ubuntu 24.04 | GCC 13 | Debug | ✅ | ✅ Full Testing |
| Ubuntu 24.04 | GCC 13 | Release | ❌ | ✅ Full Testing |
| Ubuntu 24.04 | Clang 15 | Debug | ✅ | ✅ Full Testing |
| Ubuntu 24.04 | Clang 15 | Release | ❌ | ✅ Full Testing |
| Ubuntu 20.04 | GCC 9 | Debug | ❌ | ⚠️ Build-Only |

**Additional checks:**
- Memory tracking enabled
- Code coverage analysis
- Performance regression tests

---

### Ubuntu 20.04 Testing Policy

**Build-Only Support:**
- Ubuntu 20.04 builds are verified for compatibility
- Tests are **not** run on Ubuntu 20.04 due to GitHub Actions runner availability issues
- Ubuntu 20.04 reaches end-of-life in April 2025
- Full testing is performed on Ubuntu 22.04 and 24.04

**Why Build-Only?**
- GitHub Actions Ubuntu 20.04 runners are being phased out
- Frequent pending/queued states cause CI/CD delays
- Build verification ensures compatibility without test overhead
- Users can still run tests locally on Ubuntu 20.04

**Local Testing on Ubuntu 20.04:**
```bash
# Build and test locally on Ubuntu 20.04
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

---

### View CI/CD Results

**CI/CD Dashboard:**
- [GitHub Actions Workflows](https://github.com/jwsung91/unilink/actions)
- [Coverage Reports](https://github.com/jwsung91/unilink/actions/workflows/coverage.yml)

**What CI/CD validates:**
- ✅ All unit tests pass
- ✅ No memory leaks detected
- ✅ No sanitizer errors
- ✅ Code coverage maintained
- ✅ Performance benchmarks within limits

---

## Writing Custom Tests

### Test Structure

Tests use Google Test framework:

```cpp
#include <gtest/gtest.h>
#include "unilink/unilink.hpp"

// Test fixture
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
    
    void TearDown() override {
        // Cleanup code
    }
};

// Test case
TEST_F(MyTest, BasicFunctionality) {
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .build();
    
    ASSERT_NE(client, nullptr);
    EXPECT_FALSE(client->is_connected());
}
```

---

### Example: Custom Integration Test

```cpp
#include <gtest/gtest.h>
#include "unilink/unilink.hpp"
#include <thread>
#include <chrono>

TEST(CustomTest, ClientServerCommunication) {
    std::string received_data;
    bool server_ready = false;
    
    // Create server
    auto server = unilink::tcp_server(8080)
        .unlimited_clients()
        .on_accept([](const std::string& client_id) {
            std::cout << "Client connected: " << client_id << std::endl;
        })
        .on_data([&received_data](const std::string& data) {
            received_data = data;
        })
        .build();
    
    server->start();
    server_ready = true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create client
    auto client = unilink::tcp_client("127.0.0.1", 8080)
        .build();
    
    client->start();
    
    // Wait for connection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Send data
    std::string test_data = "Hello, Server!";
    client->send(test_data);
    
    // Wait for data to be received
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify
    EXPECT_EQ(received_data, test_data);
    
    // Cleanup
    client->stop();
    server->stop();
}
```

---

### Running Custom Tests

```bash
# Add your test file to test/CMakeLists.txt

# Build
cmake --build build

# Run
./build/test/my_custom_test
```

---

## Test Configuration

### CTest Configuration

Edit `CTestTestfile.cmake` or use command-line options:

```bash
# Timeout for long-running tests
ctest --timeout 300

# Repeat tests to catch flaky behavior
ctest --repeat until-fail:100

# Run only tests matching pattern
ctest -R "TcpClient.*"

# Exclude tests matching pattern
ctest -E "Stress.*"
```

---

### Environment Variables

Control test behavior:

```bash
# Increase log verbosity
export UNILINK_LOG_LEVEL=DEBUG

# Disable colored output
export GTEST_COLOR=no

# Run specific tests
export GTEST_FILTER=TcpClientTest.*

# Run tests
ctest
```

---

## Troubleshooting Tests

### Test Failures

If tests fail:

1. **Check test output:**
   ```bash
   ctest --output-on-failure --verbose
   ```

2. **Run specific failing test:**
   ```bash
   ./build/test/run_core_tests --gtest_filter=FailingTest
   ```

3. **Check for resource issues:**
   - Port conflicts (another service using test ports)
   - Insufficient permissions
   - Network connectivity

---

### Port Conflicts

```bash
# Check if port is in use
sudo lsof -i :8080

# Kill process using port
sudo kill -9 <PID>
```

---

### Memory Issues

```bash
# Run with Valgrind for detailed analysis
valgrind --leak-check=full ./build/test/run_memory_safety_tests

# Or use AddressSanitizer
cmake -S . -B build -DUNILINK_ENABLE_SANITIZERS=ON
cmake --build build
./build/test/run_memory_safety_tests
```

---

## Performance Regression Testing

### Benchmark Baseline

Establish performance baseline:

```bash
./build/test/run_performance_tests > baseline.txt
```

### Compare Against Baseline

```bash
./build/test/run_performance_tests > current.txt
diff baseline.txt current.txt
```

### Automated Regression Detection

CI/CD automatically detects performance regressions > 10%.

---

## Code Coverage

### Generate Coverage Report

```bash
# Build with coverage flags
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage" \
  -DBUILD_TESTING=ON

cmake --build build -j

# Run tests
cd build
ctest

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --list coverage.info
```

### View HTML Coverage Report

```bash
# Generate HTML report
genhtml coverage.info --output-directory coverage_html

# Open in browser
xdg-open coverage_html/index.html
```

---

## Next Steps

- [Performance Optimization](performance.md) - Optimize your tests
- [Best Practices](best_practices.md) - Testing best practices
- [Build Guide](build_guide.md) - Build options for testing
- [Troubleshooting](troubleshooting.md) - Common test issues

