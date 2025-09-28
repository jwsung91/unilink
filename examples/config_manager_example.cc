#include <iostream>
#include <thread>
#include <chrono>
#include "unilink/unilink.hpp"

using namespace unilink;

int main() {
    std::cout << "=== unilink Configuration Manager Example ===" << std::endl;
    
    // 1. Create configuration manager with defaults
    auto config = config_manager::ConfigFactory::create_with_defaults();
    
    std::cout << "\n1. Default Configuration:" << std::endl;
    std::cout << "TCP Client Host: " << std::any_cast<std::string>(config->get("tcp.client.host")) << std::endl;
    std::cout << "TCP Client Port: " << std::any_cast<int>(config->get("tcp.client.port")) << std::endl;
    std::cout << "Serial Port: " << std::any_cast<std::string>(config->get("serial.port")) << std::endl;
    std::cout << "Logging Level: " << std::any_cast<std::string>(config->get("logging.level")) << std::endl;
    
    // 2. Modify configuration
    std::cout << "\n2. Modifying Configuration:" << std::endl;
    config->set("tcp.client.host", std::string("192.168.1.100"));
    config->set("tcp.client.port", 9090);
    config->set("serial.port", std::string("/dev/ttyACM0"));
    config->set("logging.level", std::string("debug"));
    
    std::cout << "Updated TCP Client Host: " << std::any_cast<std::string>(config->get("tcp.client.host")) << std::endl;
    std::cout << "Updated TCP Client Port: " << std::any_cast<int>(config->get("tcp.client.port")) << std::endl;
    std::cout << "Updated Serial Port: " << std::any_cast<std::string>(config->get("serial.port")) << std::endl;
    std::cout << "Updated Logging Level: " << std::any_cast<std::string>(config->get("logging.level")) << std::endl;
    
    // 3. Register change callback
    std::cout << "\n3. Change Notification:" << std::endl;
    config->on_change("tcp.client.port", [](const std::string& key, const std::any& old_value, const std::any& new_value) {
        std::cout << "Configuration changed: " << key 
                  << " from " << std::any_cast<int>(old_value) 
                  << " to " << std::any_cast<int>(new_value) << std::endl;
    });
    
    // Trigger change
    config->set("tcp.client.port", 8080);
    
    // 4. Custom validation
    std::cout << "\n4. Custom Validation:" << std::endl;
    config->register_validator("tcp.client.port", [](const std::any& value) {
        int port = std::any_cast<int>(value);
        if (port < 1 || port > 65535) {
            return config_manager::ValidationResult::error("Port must be between 1 and 65535");
        }
        return config_manager::ValidationResult::success();
    });
    
    // Test valid port
    auto result = config->set("tcp.client.port", 80);
    std::cout << "Setting port to 80: " << (result.is_valid ? "Valid" : "Invalid") << std::endl;
    
    // Test invalid port
    result = config->set("tcp.client.port", 70000);
    std::cout << "Setting port to 70000: " << (result.is_valid ? "Valid" : "Invalid") << std::endl;
    if (!result.is_valid) {
        std::cout << "Error: " << result.error_message << std::endl;
    }
    
    // 5. Save and load configuration
    std::cout << "\n5. Configuration Persistence:" << std::endl;
    std::string config_file = "/tmp/unilink_example.conf";
    
    if (config->save_to_file(config_file)) {
        std::cout << "Configuration saved to " << config_file << std::endl;
        
        // Load into new config manager
        auto loaded_config = config_manager::ConfigFactory::create();
        if (loaded_config->load_from_file(config_file)) {
            std::cout << "Configuration loaded from file" << std::endl;
            
            // Safe access with type checking
            if (loaded_config->has("tcp.client.host")) {
                auto host_value = loaded_config->get("tcp.client.host");
                if (host_value.type() == typeid(std::string)) {
                    std::cout << "Loaded TCP Client Host: " << std::any_cast<std::string>(host_value) << std::endl;
                }
            }
            
            if (loaded_config->has("tcp.client.port")) {
                auto port_value = loaded_config->get("tcp.client.port");
                if (port_value.type() == typeid(int)) {
                    std::cout << "Loaded TCP Client Port: " << std::any_cast<int>(port_value) << std::endl;
                } else if (port_value.type() == typeid(std::string)) {
                    std::cout << "Loaded TCP Client Port: " << std::any_cast<std::string>(port_value) << std::endl;
                }
            }
        } else {
            std::cout << "Failed to load configuration from file" << std::endl;
        }
    } else {
        std::cout << "Failed to save configuration to file" << std::endl;
    }
    
    // 6. Configuration introspection
    std::cout << "\n6. Configuration Introspection:" << std::endl;
    auto keys = config->get_keys();
    std::cout << "Total configuration items: " << keys.size() << std::endl;
    
    std::cout << "\nAll configuration items:" << std::endl;
    for (const auto& key : keys) {
        std::cout << "  " << key << " (" << config->get_description(key) << ")" << std::endl;
    }
    
    // 7. Thread safety demonstration
    std::cout << "\n7. Thread Safety Test:" << std::endl;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([config, i]() {
            for (int j = 0; j < 10; ++j) {
                std::string key = "thread_" + std::to_string(i) + "_key_" + std::to_string(j);
                std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                
                config->set(key, value);
                auto result = config->get(key);
                std::cout << "Thread " << i << " set " << key << " = " << std::any_cast<std::string>(result) << std::endl;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "\nThread safety test completed successfully!" << std::endl;
    
    // 8. Singleton usage
    std::cout << "\n8. Singleton Usage:" << std::endl;
    auto singleton1 = config_manager::ConfigFactory::get_singleton();
    auto singleton2 = config_manager::ConfigFactory::get_singleton();
    
    std::cout << "Singleton instances are the same: " << (singleton1 == singleton2 ? "Yes" : "No") << std::endl;
    
    std::cout << "\n=== Example completed successfully! ===" << std::endl;
    return 0;
}
