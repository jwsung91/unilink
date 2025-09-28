#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "unilink/config/iconfig_manager.hpp"
#include "unilink/config/config_manager.hpp"
#include "unilink/config/config_factory.hpp"

using namespace unilink::config;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = ConfigFactory::create();
    }

    void TearDown() override {
        config_.reset();
    }

    std::shared_ptr<ConfigManagerInterface> config_;
};

// Basic functionality tests
TEST_F(ConfigManagerTest, SetAndGetString) {
    std::string test_value = "test_string";
    config_->set("test_key", test_value);
    
    auto result = config_->get("test_key");
    EXPECT_EQ(std::any_cast<std::string>(result), test_value);
}

TEST_F(ConfigManagerTest, SetAndGetInteger) {
    int test_value = 42;
    config_->set("test_int", test_value);
    
    auto result = config_->get("test_int");
    EXPECT_EQ(std::any_cast<int>(result), test_value);
}

TEST_F(ConfigManagerTest, SetAndGetBoolean) {
    bool test_value = true;
    config_->set("test_bool", test_value);
    
    auto result = config_->get("test_bool");
    EXPECT_EQ(std::any_cast<bool>(result), test_value);
}

TEST_F(ConfigManagerTest, SetAndGetDouble) {
    double test_value = 3.14159;
    config_->set("test_double", test_value);
    
    auto result = config_->get("test_double");
    EXPECT_DOUBLE_EQ(std::any_cast<double>(result), test_value);
}

TEST_F(ConfigManagerTest, GetWithDefault) {
    std::string default_value = "default";
    auto result = config_->get("nonexistent_key", default_value);
    EXPECT_EQ(std::any_cast<std::string>(result), default_value);
}

TEST_F(ConfigManagerTest, HasKey) {
    config_->set("existing_key", std::string("value"));
    
    EXPECT_TRUE(config_->has("existing_key"));
    EXPECT_FALSE(config_->has("nonexistent_key"));
}

TEST_F(ConfigManagerTest, RemoveKey) {
    config_->set("key_to_remove", std::string("value"));
    EXPECT_TRUE(config_->has("key_to_remove"));
    
    bool removed = config_->remove("key_to_remove");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(config_->has("key_to_remove"));
}

TEST_F(ConfigManagerTest, ClearAll) {
    config_->set("key1", std::string("value1"));
    config_->set("key2", std::string("value2"));
    config_->set("key3", 123);
    
    EXPECT_EQ(config_->get_keys().size(), 3);
    
    config_->clear();
    EXPECT_EQ(config_->get_keys().size(), 0);
}

// Validation tests
TEST_F(ConfigManagerTest, ValidationSuccess) {
    ConfigItem item("test_key", std::string("test_value"), ConfigType::String, true, "Test description");
    config_->register_item(item);
    
    auto result = config_->validate("test_key");
    EXPECT_TRUE(result.is_valid);
}

TEST_F(ConfigManagerTest, ValidationFailure) {
    ConfigItem item("test_key", std::string("test_value"), ConfigType::String, true, "Test description");
    config_->register_item(item);
    
    // Try to set wrong type
    auto result = config_->set("test_key", 123);
    EXPECT_FALSE(result.is_valid);
}

// Change notification tests
TEST_F(ConfigManagerTest, ChangeNotification) {
    bool callback_called = false;
    std::string changed_key;
    std::any old_value, new_value;
    
    config_->on_change("test_key", [&](const std::string& key, const std::any& old_val, const std::any& new_val) {
        callback_called = true;
        changed_key = key;
        old_value = old_val;
        new_value = new_val;
    });
    
    config_->set("test_key", std::string("initial_value"));
    config_->set("test_key", std::string("updated_value"));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(changed_key, "test_key");
    EXPECT_EQ(std::any_cast<std::string>(old_value), "initial_value");
    EXPECT_EQ(std::any_cast<std::string>(new_value), "updated_value");
}

// File persistence tests
TEST_F(ConfigManagerTest, SaveAndLoadFile) {
    // Register items with proper types first
    ConfigItem string_item("string_key", std::string("string_value"), ConfigType::String, false, "String test");
    ConfigItem int_item("int_key", 42, ConfigType::Integer, false, "Integer test");
    ConfigItem bool_item("bool_key", true, ConfigType::Boolean, false, "Boolean test");
    ConfigItem double_item("double_key", 3.14159, ConfigType::Double, false, "Double test");
    
    config_->register_item(string_item);
    config_->register_item(int_item);
    config_->register_item(bool_item);
    config_->register_item(double_item);
    
    std::string test_file = "/tmp/test_config.conf";
    EXPECT_TRUE(config_->save_to_file(test_file));
    
    auto loaded_config = ConfigFactory::create();
    EXPECT_TRUE(loaded_config->load_from_file(test_file));
    
    EXPECT_EQ(std::any_cast<std::string>(loaded_config->get("string_key")), "string_value");
    EXPECT_EQ(std::any_cast<int>(loaded_config->get("int_key")), 42);
    EXPECT_EQ(std::any_cast<bool>(loaded_config->get("bool_key")), true);
    EXPECT_DOUBLE_EQ(std::any_cast<double>(loaded_config->get("double_key")), 3.14159);
}

// Factory tests
TEST_F(ConfigManagerTest, FactoryCreate) {
    auto config = ConfigFactory::create();
    EXPECT_NE(config, nullptr);
}

TEST_F(ConfigManagerTest, FactoryCreateWithDefaults) {
    auto config = ConfigFactory::create_with_defaults();
    EXPECT_NE(config, nullptr);
    EXPECT_TRUE(config->has("tcp.client.host"));
    EXPECT_TRUE(config->has("tcp.server.port"));
    EXPECT_TRUE(config->has("serial.port"));
    EXPECT_TRUE(config->has("logging.level"));
}

TEST_F(ConfigManagerTest, FactorySingleton) {
    auto config1 = ConfigFactory::get_singleton();
    auto config2 = ConfigFactory::get_singleton();
    
    EXPECT_EQ(config1, config2);
}

// Presets tests
TEST_F(ConfigManagerTest, TcpClientPresets) {
    auto config = ConfigFactory::create();
    ConfigPresets::setup_tcp_client_defaults(config);
    
    EXPECT_EQ(std::any_cast<std::string>(config->get("tcp.client.host")), "localhost");
    EXPECT_EQ(std::any_cast<int>(config->get("tcp.client.port")), 8080);
    EXPECT_EQ(std::any_cast<int>(config->get("tcp.client.retry_interval_ms")), 1000);
}

TEST_F(ConfigManagerTest, TcpServerPresets) {
    auto config = ConfigFactory::create();
    ConfigPresets::setup_tcp_server_defaults(config);
    
    EXPECT_EQ(std::any_cast<std::string>(config->get("tcp.server.host")), "0.0.0.0");
    EXPECT_EQ(std::any_cast<int>(config->get("tcp.server.port")), 8080);
    EXPECT_EQ(std::any_cast<int>(config->get("tcp.server.max_connections")), 100);
}

TEST_F(ConfigManagerTest, SerialPresets) {
    auto config = ConfigFactory::create();
    ConfigPresets::setup_serial_defaults(config);
    
    EXPECT_EQ(std::any_cast<std::string>(config->get("serial.port")), "/dev/ttyUSB0");
    EXPECT_EQ(std::any_cast<int>(config->get("serial.baud_rate")), 9600);
    EXPECT_EQ(std::any_cast<int>(config->get("serial.data_bits")), 8);
}

TEST_F(ConfigManagerTest, LoggingPresets) {
    auto config = ConfigFactory::create();
    ConfigPresets::setup_logging_defaults(config);
    
    EXPECT_EQ(std::any_cast<std::string>(config->get("logging.level")), "info");
    EXPECT_EQ(std::any_cast<bool>(config->get("logging.enable_console")), true);
    EXPECT_EQ(std::any_cast<bool>(config->get("logging.enable_file")), false);
}

// Thread safety tests
TEST_F(ConfigManagerTest, ThreadSafety) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string key = "thread_" + std::to_string(i) + "_key_" + std::to_string(j);
                std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                
                config_->set(key, value);
                auto result = config_->get(key);
                EXPECT_EQ(std::any_cast<std::string>(result), value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all keys were set correctly
    for (int i = 0; i < num_threads; ++i) {
        for (int j = 0; j < operations_per_thread; ++j) {
            std::string key = "thread_" + std::to_string(i) + "_key_" + std::to_string(j);
            std::string expected_value = "value_" + std::to_string(i) + "_" + std::to_string(j);
            
            EXPECT_TRUE(config_->has(key));
            auto result = config_->get(key);
            EXPECT_EQ(std::any_cast<std::string>(result), expected_value);
        }
    }
}
