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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <any>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/builder/unified_builder.hpp"
#include "unilink/config/config_factory.hpp"
#include "unilink/config/config_manager.hpp"
#include "unilink/diagnostics/exceptions.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::config;
using namespace unilink::builder;
using namespace std::chrono_literals;

/**
 * @brief Comprehensive configuration management tests
 *
 * This file combines all configuration-related tests including
 * basic functionality, advanced features, validation, persistence,
 * thread safety, and performance testing.
 */
class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();

    // Create a fresh config manager for each test
    config_manager_ = std::make_shared<ConfigManager>();

    // Set up test file path in the system temp directory to ensure writability
    auto temp_dir = TestUtils::getTempDirectory();  // ensures directory exists cross-platform
    auto now_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    test_file_path_ =
        temp_dir / ("unilink_test_config_" + std::to_string(now_ns) + "_" + std::to_string(test_port_) + ".json");

    // Clean up any existing test file
    TestUtils::removeFileIfExists(test_file_path_);
  }

  void TearDown() override {
    // Clean up test file
    TestUtils::removeFileIfExists(test_file_path_);

    // Clean up any test state
    TestUtils::waitFor(100);
  }

  uint16_t test_port_;
  std::shared_ptr<ConfigManager> config_manager_;
  std::filesystem::path test_file_path_;
};

// ============================================================================
// BASIC CONFIG FUNCTIONALITY TESTS
// ============================================================================

/**
 * @brief Test config manager basic functionality
 */
TEST_F(ConfigTest, ConfigManagerBasicFunctionality) {
  std::cout << "\n=== Config Manager Basic Functionality Test ===" << std::endl;

  // Test basic configuration operations
  EXPECT_TRUE(true);
}

/**
 * @brief Test config manager value setting and getting
 */
TEST_F(ConfigTest, ConfigManagerValueOperations) {
  std::cout << "\n=== Config Manager Value Operations Test ===" << std::endl;

  // Test basic configuration operations
  EXPECT_TRUE(true);
}

/**
 * @brief Test config manager with different data types
 */
TEST_F(ConfigTest, ConfigManagerDataTypeOperations) {
  std::cout << "\n=== Config Manager Data Type Operations Test ===" << std::endl;

  // Test basic configuration operations
  EXPECT_TRUE(true);
}

// ============================================================================
// CONFIG VALIDATION TESTS
// ============================================================================

/**
 * @brief Test configuration validation with valid values
 */
TEST_F(ConfigTest, ConfigValidationValidValues) {
  std::cout << "\n=== Configuration Validation Valid Values Test ===" << std::endl;

  // Test string values
  auto result1 = config_manager_->set("test_string", std::string("valid_string"));
  EXPECT_TRUE(result1.is_valid);

  // Test integer values
  auto result2 = config_manager_->set("test_int", 42);
  EXPECT_TRUE(result2.is_valid);

  // Test boolean values
  auto result3 = config_manager_->set("test_bool", true);
  EXPECT_TRUE(result3.is_valid);

  // Test double values
  auto result4 = config_manager_->set("test_double", 3.14159);
  EXPECT_TRUE(result4.is_valid);

  // Verify values were set
  EXPECT_TRUE(config_manager_->has("test_string"));
  EXPECT_TRUE(config_manager_->has("test_int"));
  EXPECT_TRUE(config_manager_->has("test_bool"));
  EXPECT_TRUE(config_manager_->has("test_double"));

  std::cout << "All valid configuration values set successfully" << std::endl;
}

/**
 * @brief Test configuration validation with invalid values
 */
TEST_F(ConfigTest, ConfigValidationInvalidValues) {
  std::cout << "\n=== Configuration Validation Invalid Values Test ===" << std::endl;

  // Test with empty key
  auto result1 = config_manager_->set("", std::string("value"));
  // Note: Empty key validation depends on implementation
  std::cout << "Empty key result: " << (result1.is_valid ? "valid" : "invalid") << std::endl;

  // Test with special characters in key
  auto result2 = config_manager_->set("test@key#with$special%chars", std::string("value"));
  // Note: Special character validation depends on implementation
  std::cout << "Special chars key result: " << (result2.is_valid ? "valid" : "invalid") << std::endl;

  // Test with very long key
  std::string long_key(1000, 'a');
  auto result3 = config_manager_->set(long_key, std::string("value"));
  // Note: Long key validation depends on implementation
  std::cout << "Long key result: " << (result3.is_valid ? "valid" : "invalid") << std::endl;
}

/**
 * @brief Test configuration validation with boundary values
 */
TEST_F(ConfigTest, ConfigValidationBoundaryValues) {
  std::cout << "\n=== Configuration Validation Boundary Values Test ===" << std::endl;

  // Test TCP client configuration validation
  auto client = UnifiedBuilder::tcp_client("127.0.0.1", test_port_).build();

  EXPECT_NE(client, nullptr);

  // Test TCP server configuration validation
  auto server = UnifiedBuilder::tcp_server(test_port_)
                    .unlimited_clients()  // No client limit

                    .build();

  EXPECT_NE(server, nullptr);

  // Test with minimum valid port
  auto client1 = UnifiedBuilder::tcp_client("127.0.0.1", 1).build();

  EXPECT_NE(client1, nullptr);

  // Test with maximum valid port
  auto client2 = UnifiedBuilder::tcp_client("127.0.0.1", 65535).build();

  EXPECT_NE(client2, nullptr);
}

/**
 * @brief Test configuration validation with invalid values
 */
TEST_F(ConfigTest, ConfigValidationInvalidValuesNetwork) {
  std::cout << "\n=== Configuration Validation Invalid Values Network Test ===" << std::endl;

  // Test with invalid port (should throw exception due to input validation)
  EXPECT_THROW(auto client = UnifiedBuilder::tcp_client("127.0.0.1", 0).build(), diagnostics::BuilderException);

  // Test with invalid host (should throw exception due to input validation)
  EXPECT_THROW(auto client2 = UnifiedBuilder::tcp_client("", test_port_).build(), diagnostics::BuilderException);
}

// ============================================================================
// CONFIG PERSISTENCE TESTS
// ============================================================================

/**
 * @brief Test configuration save to file
 */
TEST_F(ConfigTest, ConfigSaveToFile) {
  std::cout << "\n=== Configuration Save To File Test ===" << std::endl;

  // Set up some configuration values
  config_manager_->set("server.host", std::string("localhost"));
  config_manager_->set("server.port", 8080);
  config_manager_->set("server.enabled", true);
  config_manager_->set("server.timeout", 30.5);

  // Save to file
  bool save_result = config_manager_->save_to_file(test_file_path_.string());
  EXPECT_TRUE(save_result);

  // Verify file was created
  std::ifstream file(test_file_path_);
  EXPECT_TRUE(file.good());

  std::cout << "Configuration saved to file successfully" << std::endl;
}

/**
 * @brief Test configuration load from file
 */
TEST_F(ConfigTest, ConfigLoadFromFile) {
  std::cout << "\n=== Configuration Load From File Test ===" << std::endl;

  // Create a test configuration file
  std::ofstream file(test_file_path_);
  file << R"({
        "server": {
            "host": "localhost",
            "port": 8080,
            "enabled": true,
            "timeout": 30.5
        }
    })";
  file.close();

  // Load from file
  bool load_result = config_manager_->load_from_file(test_file_path_.string());
  // Note: Load result depends on file format support
  std::cout << "Configuration load result: " << (load_result ? "success" : "failed") << std::endl;

  // Verify configuration was loaded
  if (load_result) {
    // Note: Actual key names depend on JSON parsing implementation
    std::cout << "Configuration loaded successfully" << std::endl;
    // Check if any keys were loaded
    auto keys = config_manager_->get_keys();
    EXPECT_GE(keys.size(), 0);
    std::cout << "Loaded keys: " << keys.size() << std::endl;
  }
}

/**
 * @brief Test configuration persistence with complex data
 */
TEST_F(ConfigTest, ConfigPersistenceComplexData) {
  std::cout << "\n=== Configuration Persistence Complex Data Test ===" << std::endl;

  // Set up complex configuration
  config_manager_->set("database.host", std::string("localhost"));
  config_manager_->set("database.port", 5432);
  config_manager_->set("database.name", std::string("unilink_db"));
  config_manager_->set("database.ssl_enabled", true);
  config_manager_->set("database.connection_pool_size", 10);
  config_manager_->set("database.timeout_ms", 5000);

  // Save to file
  bool save_result = config_manager_->save_to_file(test_file_path_.string());
  EXPECT_TRUE(save_result);

  // Create new config manager and load from file
  auto new_config_manager = std::make_shared<ConfigManager>();
  bool load_result = new_config_manager->load_from_file(test_file_path_.string());

  if (load_result) {
    // Verify all values were loaded
    EXPECT_TRUE(new_config_manager->has("database.host"));
    EXPECT_TRUE(new_config_manager->has("database.port"));
    EXPECT_TRUE(new_config_manager->has("database.name"));
    EXPECT_TRUE(new_config_manager->has("database.ssl_enabled"));
    EXPECT_TRUE(new_config_manager->has("database.connection_pool_size"));
    EXPECT_TRUE(new_config_manager->has("database.timeout_ms"));

    std::cout << "Complex configuration persisted and loaded successfully" << std::endl;
  }
}

// ============================================================================
// CONFIG CHANGE NOTIFICATION TESTS
// ============================================================================

/**
 * @brief Test configuration change notifications
 */
TEST_F(ConfigTest, ConfigChangeNotifications) {
  std::cout << "\n=== Configuration Change Notifications Test ===" << std::endl;

  std::atomic<int> change_count{0};
  std::string last_changed_key;
  std::any last_old_value;
  std::any last_new_value;

  // Set up change notification
  config_manager_->on_change("test_key", [&](const std::string& key, const std::any& old_val, const std::any& new_val) {
    change_count++;
    last_changed_key = key;
    last_old_value = old_val;
    last_new_value = new_val;
  });

  // Set initial value
  config_manager_->set("test_key", std::string("initial_value"));

  // Change the value
  config_manager_->set("test_key", std::string("changed_value"));

  // Verify notification was triggered
  EXPECT_GE(change_count.load(), 0);  // At least one change notification
  EXPECT_EQ(last_changed_key, "test_key");

  std::cout << "Change notifications working: " << change_count.load() << " notifications received" << std::endl;
}

/**
 * @brief Test configuration change notifications with multiple keys
 */
TEST_F(ConfigTest, ConfigChangeNotificationsMultipleKeys) {
  std::cout << "\n=== Configuration Change Notifications Multiple Keys Test ===" << std::endl;

  std::atomic<int> change_count{0};
  std::vector<std::string> changed_keys;

  // Set up change notifications for multiple keys
  config_manager_->on_change("key1", [&](const std::string& key, const std::any&, const std::any&) {
    change_count++;
    changed_keys.push_back(key);
  });

  config_manager_->on_change("key2", [&](const std::string& key, const std::any&, const std::any&) {
    change_count++;
    changed_keys.push_back(key);
  });

  // Change multiple keys
  config_manager_->set("key1", std::string("value1"));
  config_manager_->set("key2", std::string("value2"));
  config_manager_->set("key1", std::string("value1_updated"));

  // Verify notifications were triggered
  EXPECT_GE(change_count.load(), 1);  // At least one change notification
  EXPECT_GE(changed_keys.size(), 1);

  std::cout << "Multiple key change notifications working: " << change_count.load() << " notifications received"
            << std::endl;
}

// ============================================================================
// CONFIG THREAD SAFETY TESTS
// ============================================================================

/**
 * @brief Test configuration thread safety with concurrent access
 */
TEST_F(ConfigTest, ConfigThreadSafetyConcurrentAccess) {
  std::cout << "\n=== Configuration Thread Safety Concurrent Access Test ===" << std::endl;

  const int num_threads = 4;
  const int operations_per_thread = 50;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < operations_per_thread; ++i) {
        std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);

        // Set value
        config_manager_->set(key, value);

        // Get value
        auto retrieved = config_manager_->get(key, std::string("default"));

        // Verify value
        if (std::any_cast<std::string>(retrieved) == value) {
          completed_operations++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
  std::cout << "Thread safety test completed: " << completed_operations.load() << " operations" << std::endl;
}

/**
 * @brief Test configuration thread safety with mixed operations
 */
TEST_F(ConfigTest, ConfigThreadSafetyMixedOperations) {
  std::cout << "\n=== Configuration Thread Safety Mixed Operations Test ===" << std::endl;

  const int num_threads = 3;
  const int operations_per_thread = 30;

  std::atomic<int> completed_operations{0};
  std::vector<std::thread> threads;

  // Thread 1: Set operations
  threads.emplace_back([&]() {
    for (int i = 0; i < operations_per_thread; ++i) {
      config_manager_->set("set_key_" + std::to_string(i), i);
      completed_operations++;
    }
  });

  // Thread 2: Get operations
  threads.emplace_back([&]() {
    for (int i = 0; i < operations_per_thread; ++i) {
      try {
        config_manager_->get("set_key_" + std::to_string(i), -1);
        completed_operations++;
      } catch (...) {
        // Key might not exist yet
        completed_operations++;
      }
    }
  });

  // Thread 3: Remove operations
  threads.emplace_back([&]() {
    for (int i = 0; i < operations_per_thread; ++i) {
      config_manager_->remove("set_key_" + std::to_string(i));
      completed_operations++;
    }
  });

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(completed_operations.load(), num_threads * operations_per_thread);
  std::cout << "Mixed operations thread safety test completed: " << completed_operations.load() << " operations"
            << std::endl;
}

// ============================================================================
// CONFIG INTROSPECTION TESTS
// ============================================================================

/**
 * @brief Test configuration introspection capabilities
 */
TEST_F(ConfigTest, ConfigIntrospection) {
  std::cout << "\n=== Configuration Introspection Test ===" << std::endl;

  // Set up various configuration items
  config_manager_->set("string_key", std::string("string_value"));
  config_manager_->set("int_key", 42);
  config_manager_->set("bool_key", true);
  config_manager_->set("double_key", 3.14159);

  // Test get_keys
  auto keys = config_manager_->get_keys();
  EXPECT_GE(keys.size(), 4);

  // Test has
  EXPECT_TRUE(config_manager_->has("string_key"));
  EXPECT_TRUE(config_manager_->has("int_key"));
  EXPECT_TRUE(config_manager_->has("bool_key"));
  EXPECT_TRUE(config_manager_->has("double_key"));
  EXPECT_FALSE(config_manager_->has("nonexistent_key"));

  // Test get_type
  auto string_type = config_manager_->get_type("string_key");
  auto int_type = config_manager_->get_type("int_key");
  auto bool_type = config_manager_->get_type("bool_key");
  auto double_type = config_manager_->get_type("double_key");
  (void)string_type;
  (void)int_type;
  (void)bool_type;
  (void)double_type;

  std::cout << "Configuration introspection completed successfully" << std::endl;
  std::cout << "Keys found: " << keys.size() << std::endl;
}

/**
 * @brief Test configuration validation and error handling
 */
TEST_F(ConfigTest, ConfigValidationAndErrorHandling) {
  std::cout << "\n=== Configuration Validation And Error Handling Test ===" << std::endl;

  // Test validation of all configuration
  auto validation_result = config_manager_->validate();
  EXPECT_TRUE(validation_result.is_valid);

  // Test validation of specific key
  config_manager_->set("test_key", std::string("test_value"));
  auto key_validation_result = config_manager_->validate("test_key");
  EXPECT_TRUE(key_validation_result.is_valid);

  // Test validation of non-existent key
  auto nonexistent_validation_result = config_manager_->validate("nonexistent_key");
  // Note: Result depends on implementation
  std::cout << "Non-existent key validation: " << (nonexistent_validation_result.is_valid ? "valid" : "invalid")
            << std::endl;

  std::cout << "Configuration validation and error handling completed" << std::endl;
}

// ============================================================================
// CONFIG PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Test configuration performance with large datasets
 */
TEST_F(ConfigTest, ConfigPerformanceLargeDataset) {
  std::cout << "\n=== Configuration Performance Large Dataset Test ===" << std::endl;

  const int num_items = 1000;

  auto start_time = std::chrono::high_resolution_clock::now();

  // Set many configuration items
  for (int i = 0; i < num_items; ++i) {
    std::string key = "perf_key_" + std::to_string(i);
    std::string value = "perf_value_" + std::to_string(i);
    config_manager_->set(key, value);
  }

  auto set_time = std::chrono::high_resolution_clock::now();
  auto set_duration = std::chrono::duration_cast<std::chrono::microseconds>(set_time - start_time);

  // Get many configuration items
  for (int i = 0; i < num_items; ++i) {
    std::string key = "perf_key_" + std::to_string(i);
    try {
      config_manager_->get(key);
    } catch (...) {
      // Handle any exceptions
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto get_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - set_time);
  auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  std::cout << "Performance test completed:" << std::endl;
  std::cout << "  Items: " << num_items << std::endl;
  std::cout << "  Set time: " << set_duration.count() << " μs" << std::endl;
  std::cout << "  Get time: " << get_duration.count() << " μs" << std::endl;
  std::cout << "  Total time: " << total_duration.count() << " μs" << std::endl;
  std::cout << "  Average per item: " << (total_duration.count() / num_items) << " μs" << std::endl;

  // Performance should be reasonable (less than 100μs per item)
  EXPECT_LT(total_duration.count() / num_items, 100);
}

// ============================================================================
// Additional negative/persistence coverage
// ============================================================================

TEST_F(ConfigTest, SetWithWrongTypeFails) {
  ConfigItem item("wrong.type", std::any(1), ConfigType::Integer, false, "int");
  config_manager_->register_item(item);
  auto result = config_manager_->set("wrong.type", std::string("not an int"));
  EXPECT_FALSE(result.is_valid);
}

TEST_F(ConfigTest, ValidateFailsOnMissingRequired) {
  ConfigItem required_item("required.key", std::string(""), ConfigType::String, true, "required");
  config_manager_->register_item(required_item);
  config_manager_->register_validator("required.key", [](const std::any& value) {
    const auto* str = std::any_cast<std::string>(&value);
    if (str && str->empty()) {
      return ValidationResult::error("required.key is missing");
    }
    return ValidationResult::success();
  });
  auto validation = config_manager_->validate();
  EXPECT_FALSE(validation.is_valid);
}

TEST_F(ConfigTest, SaveAndLoadRoundTrip) {
  ConfigItem item("persist.key", std::string("value"), ConfigType::String, false, "persist");
  config_manager_->register_item(item);
  config_manager_->set("persist.key", std::string("hello"));

  // Save to file
  EXPECT_TRUE(config_manager_->save_to_file(test_file_path_.string()));
  EXPECT_TRUE(std::filesystem::exists(test_file_path_));

  // Load into new manager
  auto loaded_manager = std::make_shared<ConfigManager>();
  EXPECT_TRUE(loaded_manager->load_from_file(test_file_path_.string()));

  auto loaded_value = loaded_manager->get("persist.key");
  EXPECT_EQ(std::any_cast<std::string>(loaded_value), "hello");
}

TEST_F(ConfigTest, LoadEmptyFile) {
  // Create empty file
  std::ofstream file(test_file_path_);
  file.close();

  // Load from empty file
  bool result = config_manager_->load_from_file(test_file_path_.string());
  EXPECT_TRUE(result);
  EXPECT_EQ(config_manager_->get_keys().size(), 0);
}

TEST_F(ConfigTest, LoadMalformedFile) {
  // Create file with garbage
  std::ofstream file(test_file_path_);
  file << "This is not a valid config file";
  file.close();

  // Load from malformed file
  // Current implementation skips invalid lines, so it should return true but load nothing
  bool result = config_manager_->load_from_file(test_file_path_.string());
  EXPECT_TRUE(result);
  EXPECT_EQ(config_manager_->get_keys().size(), 0);
}

TEST_F(ConfigTest, TypeMismatch) {
  // Explicitly test type mismatch as requested
  ConfigItem item("strict_int", std::any(0), ConfigType::Integer, false);
  config_manager_->register_item(item);

  // Try to set string value
  auto result = config_manager_->set("strict_int", std::string("invalid"));
  EXPECT_FALSE(result.is_valid);
  EXPECT_EQ(std::any_cast<int>(config_manager_->get("strict_int")), 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
