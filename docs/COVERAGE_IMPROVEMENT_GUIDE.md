# Code Coverage Improvement Guide

## Current Status

**Baseline Coverage: 47.3%**
- Lines: 1,917 / 4,049 covered
- Functions: 449 / 1,280 covered

---

## Low Coverage Areas

| Component | Current Coverage | Priority | Difficulty |
|-----------|-----------------|----------|------------|
| Input Validator | 13.8% | 🔴 HIGH | ⭐ Easy |
| Config Manager | 17.3% | 🔴 HIGH | ⭐⭐ Medium |
| Builders | 17-27% | 🔴 HIGH | ⭐ Easy |
| Logger | 18.5% | 🟡 MEDIUM | ⭐⭐ Medium |
| Transport Layer | 19-35% | 🟡 MEDIUM | ⭐⭐⭐ Hard |
| Error Handler | 30.2% | 🟢 LOW | ⭐ Easy |

---

## Quick Wins (Expected +15-20% Coverage)

### 1. Input Validator Tests (**+3% coverage**)

**File to create:** `test/test_input_validator_coverage.cc`

```cpp
#include <gtest/gtest.h>
#include "unilink/common/input_validator.hpp"
#include "unilink/common/exceptions.hpp"

using namespace unilink::common;

TEST(InputValidatorTest, PortValidation) {
    // Valid ports
    EXPECT_NO_THROW(InputValidator::validate_port(8080));
    EXPECT_NO_THROW(InputValidator::validate_port(65535));
    
    // Invalid ports  
    EXPECT_THROW(InputValidator::validate_port(0), ValidationException);
    EXPECT_THROW(InputValidator::validate_port(70000), ValidationException);
}

TEST(InputValidatorTest, BaudRateValidation) {
    // Valid baud rates
    EXPECT_NO_THROW(InputValidator::validate_baud_rate(9600));
    EXPECT_NO_THROW(InputValidator::validate_baud_rate(115200));
    
    // Invalid baud rates
    EXPECT_THROW(InputValidator::validate_baud_rate(123), ValidationException);
}

TEST(InputValidatorTest, HostValidation) {
    EXPECT_NO_THROW(InputValidator::validate_host("localhost"));
    EXPECT_NO_THROW(InputValidator::validate_host("127.0.0.1"));
    EXPECT_THROW(InputValidator::validate_host(""), ValidationException);
}

TEST(InputValidatorTest, DevicePathValidation) {
    EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyUSB0"));
    EXPECT_THROW(InputValidator::validate_device_path(""), ValidationException);
}

TEST(InputValidatorTest, BufferSizeValidation) {
    EXPECT_NO_THROW(InputValidator::validate_buffer_size(1024));
    EXPECT_THROW(InputValidator::validate_buffer_size(0), ValidationException);
    EXPECT_THROW(InputValidator::validate_buffer_size(1024*1024*1024), ValidationException);
}
```

---

### 2. Logger Tests (**+4% coverage**)

**File to create:** `test/test_logger_coverage.cc`

```cpp
#include <gtest/gtest.h>
#include "unilink/common/logger.hpp"
#include <fstream>

using namespace unilink::common;

class LoggerCoverageTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Reset logger state
        auto& logger = Logger::instance();
        logger.set_level(LogLevel::INFO);
        logger.set_file_output(false, "");
        logger.set_console_output(true);
    }
};

TEST_F(LoggerCoverageTest, AllLogLevels) {
    auto& logger = Logger::instance();
    logger.set_level(LogLevel::DEBUG);
    
    UNILINK_LOG_DEBUG("test", "op", "debug message");
    UNILINK_LOG_INFO("test", "op", "info message");
    UNILINK_LOG_WARNING("test", "op", "warning message");
    UNILINK_LOG_ERROR("test", "op", "error message");
    UNILINK_LOG_CRITICAL("test", "op", "critical message");
    
    auto stats = logger.get_stats();
    EXPECT_GT(stats.total_messages, 0);
}

TEST_F(LoggerCoverageTest, FileOutput) {
    auto& logger = Logger::instance();
    const std::string log_file = "/tmp/test_log.txt";
    
    logger.set_file_output(true, log_file);
    UNILINK_LOG_INFO("test", "file", "test message");
    logger.flush();
    
    // Verify file exists
    std::ifstream file(log_file);
    EXPECT_TRUE(file.good());
    
    logger.set_file_output(false, "");
}

TEST_F(LoggerCoverageTest, LogFiltering) {
    auto& logger = Logger::instance();
    logger.set_level(LogLevel::ERROR);
    
    logger.clear_stats();
    UNILINK_LOG_DEBUG("test", "op", "should not appear");
    UNILINK_LOG_INFO("test", "op", "should not appear");
    UNILINK_LOG_ERROR("test", "op", "should appear");
    
    auto stats = logger.get_stats();
    EXPECT_EQ(stats.total_messages, 1);
}

TEST_F(LoggerCoverageTest, LogRotation) {
    auto& logger = Logger::instance();
    logger.enable_log_rotation(true, 1024, 3);
    
    for(int i = 0; i < 100; i++) {
        UNILINK_LOG_INFO("test", "rotation", 
                        "message " + std::to_string(i) + " with enough text to fill buffer");
    }
    
    logger.enable_log_rotation(false, 0, 0);
}

TEST_F(LoggerCoverageTest, AsyncLogging) {
    auto& logger = Logger::instance();
    
    AsyncLogConfig config;
    config.queue_size = 1000;
    config.batch_size = 10;
    config.flush_interval_ms = 100;
    
    logger.enable_async_logging(true, config);
    
    for(int i = 0; i < 50; i++) {
        UNILINK_LOG_INFO("test", "async", "async message " + std::to_string(i));
    }
    
    logger.flush();
    logger.enable_async_logging(false);
}
```

---

### 3. Config Manager Tests (**+5% coverage**)

**File to create:** `test/test_config_manager_coverage.cc`

```cpp
#include <gtest/gtest.h>
#include "unilink/config/config_manager.hpp"
#include <fstream>

using namespace unilink::config;

class ConfigManagerCoverageTest : public ::testing::Test {
protected:
    ConfigManager manager_;
    const std::string test_file_ = "/tmp/test_config.txt";
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
};

TEST_F(ConfigManagerCoverageTest, SaveAndLoad) {
    // Set various types
    manager_.set("string_key", std::string("test_value"));
    manager_.set("int_key", 42);
    manager_.set("double_key", 3.14);
    manager_.set("bool_key", true);
    
    // Save
    EXPECT_TRUE(manager_.save_to_file(test_file_));
    
    // Load into new manager
    ConfigManager manager2;
    EXPECT_TRUE(manager2.load_from_file(test_file_));
    
    // Verify
    EXPECT_EQ(manager2.get<std::string>("string_key"), "test_value");
    EXPECT_EQ(manager2.get<int>("int_key"), 42);
    EXPECT_DOUBLE_EQ(manager2.get<double>("double_key"), 3.14);
    EXPECT_EQ(manager2.get<bool>("bool_key"), true);
}

TEST_F(ConfigManagerCoverageTest, ChangeCallbacks) {
    bool callback_called = false;
    std::string changed_key;
    
    manager_.register_change_callback("test", 
        [&](const std::string& key, const std::any&, const std::any&) {
            callback_called = true;
            changed_key = key;
        });
    
    manager_.set("test", std::string("value"));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(changed_key, "test");
}

TEST_F(ConfigManagerCoverageTest, KeyIntrospection) {
    manager_.set("key1", std::string("value1"));
    manager_.set("key2", 42);
    manager_.set("key3", true);
    
    auto keys = manager_.get_keys();
    EXPECT_EQ(keys.size(), 3);
    
    EXPECT_TRUE(manager_.has("key1"));
    EXPECT_FALSE(manager_.has("nonexistent"));
}

TEST_F(ConfigManagerCoverageTest, ErrorHandling) {
    // Get non-existent key
    EXPECT_THROW(manager_.get<std::string>("nonexistent"), std::runtime_error);
    
    // Wrong type
    manager_.set("int_value", 42);
    EXPECT_THROW(manager_.get<std::string>("int_value"), std::bad_any_cast);
}
```

---

### 4. Error Handler Tests (**+3% coverage**)

**File to create:** `test/test_error_handler_extended.cc`

```cpp
#include <gtest/gtest.h>
#include "unilink/common/error_handler.hpp"

using namespace unilink::common;
using namespace unilink::common::error_reporting;

class ErrorHandlerExtendedTest : public ::testing::Test {
protected:
    void TearDown() override {
        ErrorHandler::instance().reset_stats();
        ErrorHandler::instance().clear_callbacks();
    }
};

TEST_F(ErrorHandlerExtendedTest, AllErrorCategories) {
    // Test all error reporting functions
    report_connection_error("test", "connect", 
                           boost::system::error_code(), true);
    report_communication_error("test", "write", "test error");
    report_configuration_error("test", "validate", "test error");
    report_memory_error("test", "allocate", "test error");
    report_system_error("test", "init", "test error");
    report_warning("test", "op", "warning");
    report_info("test", "op", "info");
    
    auto stats = ErrorHandler::instance().get_error_stats();
    EXPECT_GT(stats.total_errors, 0);
}

TEST_F(ErrorHandlerExtendedTest, ErrorFiltering) {
    ErrorHandler::instance().set_min_error_level(ErrorLevel::ERROR);
    ErrorHandler::instance().reset_stats();
    
    report_warning("test", "op", "warning"); // Filtered
    report_error("test", "op", "error");     // Counted
    
    auto stats = ErrorHandler::instance().get_error_stats();
    EXPECT_EQ(stats.total_errors, 1);
}

TEST_F(ErrorHandlerExtendedTest, ErrorCallbacks) {
    int callback_count = 0;
    
    ErrorHandler::instance().register_callback(
        [&](const ErrorInfo& error) {
            callback_count++;
        });
    
    report_error("test", "op", "error1");
    report_error("test", "op", "error2");
    
    EXPECT_EQ(callback_count, 2);
}

TEST_F(ErrorHandlerExtendedTest, ComponentErrors) {
    report_connection_error("tcp_client", "connect", 
                           boost::system::error_code(), true);
    report_connection_error("serial", "open", 
                           boost::system::error_code(), true);
    
    auto tcp_errors = ErrorHandler::instance().get_errors_by_component("tcp_client");
    auto serial_errors = ErrorHandler::instance().get_errors_by_component("serial");
    
    EXPECT_EQ(tcp_errors.size(), 1);
    EXPECT_EQ(serial_errors.size(), 1);
}
```

---

## Implementation Steps

### Step 1: Add Test Files

```bash
# 1. Create new test files
touch test/test_input_validator_coverage.cc
touch test/test_logger_coverage.cc
touch test/test_config_manager_coverage.cc
touch test/test_error_handler_extended.cc
```

### Step 2: Update CMakeLists.txt

Add these to `test/CMakeLists.txt`:

```cmake
# Input Validator Coverage Tests
add_executable(run_input_validator_coverage_tests
    test_input_validator_coverage.cc
    test_utils.hpp
)
target_link_libraries(run_input_validator_coverage_tests
    PRIVATE ${COMMON_TEST_LIBS}
)
gtest_discover_tests(run_input_validator_coverage_tests)

# Logger Coverage Tests
add_executable(run_logger_coverage_tests
    test_logger_coverage.cc
    test_utils.hpp
)
target_link_libraries(run_logger_coverage_tests
    PRIVATE ${COMMON_TEST_LIBS}
)
gtest_discover_tests(run_logger_coverage_tests)

# Config Manager Coverage Tests
add_executable(run_config_manager_coverage_tests
    test_config_manager_coverage.cc
    test_utils.hpp
)
target_link_libraries(run_config_manager_coverage_tests
    PRIVATE ${COMMON_TEST_LIBS}
)
gtest_discover_tests(run_config_manager_coverage_tests)

# Error Handler Extended Tests
add_executable(run_error_handler_extended_tests
    test_error_handler_extended.cc
    test_utils.hpp
)
target_link_libraries(run_error_handler_extended_tests
    PRIVATE ${COMMON_TEST_LIBS}
)
gtest_discover_tests(run_error_handler_extended_tests)
```

### Step 3: Build and Test

```bash
cmake -S . -B build-coverage \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
  -DBUILD_TESTING=ON

cmake --build build-coverage -j$(nproc)
cd build-coverage && ctest

# Generate coverage
lcov --directory . --capture --output-file coverage.info --ignore-errors negative
lcov --remove coverage.info '/usr/*' '*/test/*' '*/googletest/*' --output-file filtered.info
lcov --list filtered.info
```

### Step 4: Verify Improvement

Expected results after implementing all tests:
- **Before:** 47.3% (1,917 / 4,049 lines)
- **After:** 65-70% (2,630+ / 4,049 lines)
- **Improvement:** +18-23%

---

## Medium-Term Improvements (Expected +10-15%)

### 1. Transport Layer Integration Tests

Focus on testing error scenarios and edge cases:
- Connection failures and retries
- Timeout handling
- Large data transfers
- Concurrent operations

### 2. Config File Format Tests

Test different configuration formats:
- Valid/invalid config files
- Missing keys
- Type mismatches
- Boundary values

---

## Long-Term Goals

| Milestone | Target Coverage | Timeline |
|-----------|----------------|----------|
| Phase 1 | 65% | 1-2 weeks |
| Phase 2 | 75% | 1 month |
| Phase 3 | 85%+ | 2-3 months |

---

## Coverage Metrics

Track these metrics over time:
- Line coverage
- Function coverage  
- Branch coverage (when available)
- Test execution time
- Test count

---

## Automation

The Coverage workflow runs automatically on:
- Every PR
- Every push to main/develop
- Reports are available in GitHub Actions artifacts
- PR comments show coverage changes

---

## Best Practices

1. **Write tests first** for new features (TDD)
2. **Focus on critical paths** before edge cases
3. **Test error conditions** not just happy paths
4. **Keep tests simple** and focused
5. **Use mocks** for external dependencies
6. **Measure regularly** to track progress

---

## Resources

- [Coverage workflow](../.github/workflows/coverage.yml)
- [Test utilities](../test/test_utils.hpp)
- [Existing tests](../test/)
- [Builder example](../test/test_builder_coverage.cc)

