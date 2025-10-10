#include <gtest/gtest.h>

#include <any>
#include <fstream>
#include <memory>
#include <string>

#include "unilink/config/config_manager.hpp"

using namespace unilink::config;

/**
 * @brief Config Coverage Test - ConfigManager 커버리지 확보
 */
class ConfigCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    config_manager_ = std::make_shared<ConfigManager>();
    test_file_ = "/tmp/unilink_coverage_test.json";
    std::remove(test_file_.c_str());
  }

  void TearDown() override { std::remove(test_file_.c_str()); }

  std::shared_ptr<ConfigManager> config_manager_;
  std::string test_file_;
};

// ============================================================================
// BASIC OPERATIONS
// ============================================================================

TEST_F(ConfigCoverageTest, SetAndGetStringValue) {
  // Register a string config item
  ConfigItem item;
  item.key = "test.string";
  item.type = ConfigType::String;
  item.value = std::string("default");
  item.description = "Test string";
  item.required = false;

  config_manager_->register_item(item);

  // Set a value
  auto result = config_manager_->set("test.string", std::string("hello"));
  EXPECT_TRUE(result.is_valid);

  // Get the value
  auto value = config_manager_->get("test.string");
  EXPECT_EQ(std::any_cast<std::string>(value), "hello");
}

TEST_F(ConfigCoverageTest, SetAndGetIntValue) {
  ConfigItem item;
  item.key = "test.int";
  item.type = ConfigType::Integer;
  item.value = 42;
  item.description = "Test integer";
  item.required = false;

  config_manager_->register_item(item);

  auto result = config_manager_->set("test.int", 123);
  EXPECT_TRUE(result.is_valid);

  auto value = config_manager_->get("test.int");
  EXPECT_EQ(std::any_cast<int>(value), 123);
}

TEST_F(ConfigCoverageTest, SetAndGetDoubleValue) {
  ConfigItem item;
  item.key = "test.double";
  item.type = ConfigType::Double;
  item.value = 3.14;
  item.description = "Test double";
  item.required = false;

  config_manager_->register_item(item);

  auto result = config_manager_->set("test.double", 2.71);
  EXPECT_TRUE(result.is_valid);

  auto value = config_manager_->get("test.double");
  EXPECT_DOUBLE_EQ(std::any_cast<double>(value), 2.71);
}

TEST_F(ConfigCoverageTest, SetAndGetBoolValue) {
  ConfigItem item;
  item.key = "test.bool";
  item.type = ConfigType::Boolean;
  item.value = false;
  item.description = "Test boolean";
  item.required = false;

  config_manager_->register_item(item);

  auto result = config_manager_->set("test.bool", true);
  EXPECT_TRUE(result.is_valid);

  auto value = config_manager_->get("test.bool");
  EXPECT_TRUE(std::any_cast<bool>(value));
}

// ============================================================================
// HAS & REMOVE
// ============================================================================

TEST_F(ConfigCoverageTest, HasKey) {
  ConfigItem item;
  item.key = "test.exists";
  item.type = ConfigType::String;
  item.value = std::string("value");

  config_manager_->register_item(item);
  config_manager_->set("test.exists", std::string("test"));

  EXPECT_TRUE(config_manager_->has("test.exists"));
  EXPECT_FALSE(config_manager_->has("test.not_exists"));
}

TEST_F(ConfigCoverageTest, RemoveKey) {
  ConfigItem item;
  item.key = "test.remove";
  item.type = ConfigType::String;
  item.value = std::string("value");

  config_manager_->register_item(item);
  config_manager_->set("test.remove", std::string("test"));

  EXPECT_TRUE(config_manager_->has("test.remove"));

  bool removed = config_manager_->remove("test.remove");
  EXPECT_TRUE(removed);
  EXPECT_FALSE(config_manager_->has("test.remove"));
}

TEST_F(ConfigCoverageTest, Clear) {
  // Add multiple items
  for (int i = 0; i < 5; i++) {
    ConfigItem item;
    item.key = "test.clear" + std::to_string(i);
    item.type = ConfigType::Integer;
    item.value = i;

    config_manager_->register_item(item);
    config_manager_->set(item.key, i);
  }

  config_manager_->clear();

  for (int i = 0; i < 5; i++) {
    EXPECT_FALSE(config_manager_->has("test.clear" + std::to_string(i)));
  }
}

// ============================================================================
// DEFAULT VALUES
// ============================================================================

TEST_F(ConfigCoverageTest, GetWithDefaultValue) {
  // Get non-existent key with default
  auto value = config_manager_->get("non.existent", std::string("default_value"));
  EXPECT_EQ(std::any_cast<std::string>(value), "default_value");
}

// ============================================================================
// VALIDATION
// ============================================================================

TEST_F(ConfigCoverageTest, RegisterValidator) {
  ConfigItem item;
  item.key = "test.validated";
  item.type = ConfigType::Integer;
  item.value = 50;

  config_manager_->register_item(item);

  // Register validator: value must be between 0 and 100
  config_manager_->register_validator("test.validated", [](const std::any& value) -> ValidationResult {
    int val = std::any_cast<int>(value);
    if (val < 0 || val > 100) {
      return ValidationResult{false, "Value must be between 0 and 100"};
    }
    return ValidationResult{true, ""};
  });

  // Valid value
  auto result1 = config_manager_->set("test.validated", 50);
  EXPECT_TRUE(result1.is_valid);

  // Invalid value
  auto result2 = config_manager_->set("test.validated", 150);
  EXPECT_FALSE(result2.is_valid);
}

TEST_F(ConfigCoverageTest, ValidateAll) {
  ConfigItem item1;
  item1.key = "test.req1";
  item1.type = ConfigType::String;
  item1.required = true;

  ConfigItem item2;
  item2.key = "test.req2";
  item2.type = ConfigType::Integer;
  item2.required = false;

  config_manager_->register_item(item1);
  config_manager_->register_item(item2);

  // Should fail because required item is not set
  auto result1 = config_manager_->validate();
  EXPECT_FALSE(result1.is_valid);

  // Set required item
  config_manager_->set("test.req1", std::string("value"));

  // Should pass now
  auto result2 = config_manager_->validate();
  EXPECT_TRUE(result2.is_valid);
}

TEST_F(ConfigCoverageTest, ValidateSpecificKey) {
  ConfigItem item;
  item.key = "test.specific";
  item.type = ConfigType::Integer;
  item.required = true;

  config_manager_->register_item(item);

  // Should fail because required value is not set
  auto result1 = config_manager_->validate("test.specific");
  EXPECT_FALSE(result1.is_valid);

  config_manager_->set("test.specific", 42);

  // Should pass now
  auto result2 = config_manager_->validate("test.specific");
  EXPECT_TRUE(result2.is_valid);
}

// ============================================================================
// CHANGE CALLBACKS
// ============================================================================

TEST_F(ConfigCoverageTest, ChangeCallback) {
  ConfigItem item;
  item.key = "test.callback";
  item.type = ConfigType::String;
  item.value = std::string("initial");

  config_manager_->register_item(item);

  bool callback_called = false;
  std::string old_val, new_val;

  config_manager_->on_change("test.callback",
                             [&](const std::string& key, const std::any& old_value, const std::any& new_value) {
                               callback_called = true;
                               old_val = std::any_cast<std::string>(old_value);
                               new_val = std::any_cast<std::string>(new_value);
                             });

  config_manager_->set("test.callback", std::string("initial"));
  config_manager_->set("test.callback", std::string("updated"));

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(old_val, "initial");
  EXPECT_EQ(new_val, "updated");
}

TEST_F(ConfigCoverageTest, RemoveChangeCallback) {
  ConfigItem item;
  item.key = "test.remove_callback";
  item.type = ConfigType::Integer;
  item.value = 0;

  config_manager_->register_item(item);

  int callback_count = 0;

  config_manager_->on_change("test.remove_callback",
                             [&](const std::string&, const std::any&, const std::any&) { callback_count++; });

  config_manager_->set("test.remove_callback", 1);
  EXPECT_EQ(callback_count, 1);

  config_manager_->remove_change_callback("test.remove_callback");

  config_manager_->set("test.remove_callback", 2);
  EXPECT_EQ(callback_count, 1);  // Should not increase
}

// ============================================================================
// PERSISTENCE
// ============================================================================

TEST_F(ConfigCoverageTest, SaveToFile) {
  ConfigItem item1;
  item1.key = "test.save1";
  item1.type = ConfigType::String;
  item1.value = std::string("value1");

  ConfigItem item2;
  item2.key = "test.save2";
  item2.type = ConfigType::Integer;
  item2.value = 42;

  config_manager_->register_item(item1);
  config_manager_->register_item(item2);

  config_manager_->set("test.save1", std::string("saved_value"));
  config_manager_->set("test.save2", 123);

  bool saved = config_manager_->save_to_file(test_file_);
  EXPECT_TRUE(saved);

  // Check file exists
  std::ifstream file(test_file_);
  EXPECT_TRUE(file.good());
}

TEST_F(ConfigCoverageTest, LoadFromFile) {
  // First, save config
  ConfigItem item1;
  item1.key = "test.load1";
  item1.type = ConfigType::String;
  item1.value = std::string("default");

  ConfigItem item2;
  item2.key = "test.load2";
  item2.type = ConfigType::Integer;
  item2.value = 0;

  config_manager_->register_item(item1);
  config_manager_->register_item(item2);

  config_manager_->set("test.load1", std::string("loaded_value"));
  config_manager_->set("test.load2", 456);

  config_manager_->save_to_file(test_file_);

  // Create new config manager and load
  auto new_config = std::make_shared<ConfigManager>();
  new_config->register_item(item1);
  new_config->register_item(item2);

  bool loaded = new_config->load_from_file(test_file_);
  EXPECT_TRUE(loaded);

  auto value1 = new_config->get("test.load1");
  auto value2 = new_config->get("test.load2");

  EXPECT_EQ(std::any_cast<std::string>(value1), "loaded_value");
  EXPECT_EQ(std::any_cast<int>(value2), 456);
}

// ============================================================================
// INTROSPECTION
// ============================================================================

TEST_F(ConfigCoverageTest, GetKeys) {
  ConfigItem item1;
  item1.key = "test.keys1";
  item1.type = ConfigType::String;

  ConfigItem item2;
  item2.key = "test.keys2";
  item2.type = ConfigType::Integer;

  config_manager_->register_item(item1);
  config_manager_->register_item(item2);

  auto keys = config_manager_->get_keys();
  EXPECT_GE(keys.size(), 2u);
}

TEST_F(ConfigCoverageTest, GetType) {
  ConfigItem item;
  item.key = "test.type";
  item.type = ConfigType::Double;

  config_manager_->register_item(item);

  auto type = config_manager_->get_type("test.type");
  EXPECT_EQ(type, ConfigType::Double);
}

TEST_F(ConfigCoverageTest, GetDescription) {
  ConfigItem item;
  item.key = "test.desc";
  item.type = ConfigType::String;
  item.description = "This is a test description";

  config_manager_->register_item(item);

  auto desc = config_manager_->get_description("test.desc");
  EXPECT_EQ(desc, "This is a test description");
}

TEST_F(ConfigCoverageTest, IsRequired) {
  ConfigItem item1;
  item1.key = "test.required";
  item1.type = ConfigType::String;
  item1.required = true;

  ConfigItem item2;
  item2.key = "test.optional";
  item2.type = ConfigType::String;
  item2.required = false;

  config_manager_->register_item(item1);
  config_manager_->register_item(item2);

  EXPECT_TRUE(config_manager_->is_required("test.required"));
  EXPECT_FALSE(config_manager_->is_required("test.optional"));
}
