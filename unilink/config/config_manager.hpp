#pragma once

#include "iconfig_manager.hpp"
#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>

namespace unilink {
namespace config {

/**
 * Thread-safe configuration manager implementation
 */
class ConfigManager : public ConfigManagerInterface {
public:
    ConfigManager() = default;
    ~ConfigManager() = default;
    
    // Configuration access
    std::any get(const std::string& key) const override;
    std::any get(const std::string& key, const std::any& default_value) const override;
    bool has(const std::string& key) const override;
    
    // Configuration modification
    ValidationResult set(const std::string& key, const std::any& value) override;
    bool remove(const std::string& key) override;
    void clear() override;
    
    // Configuration validation
    ValidationResult validate() const override;
    ValidationResult validate(const std::string& key) const override;
    
    // Configuration registration
    void register_item(const ConfigItem& item) override;
    void register_validator(const std::string& key, 
                           std::function<ValidationResult(const std::any&)> validator) override;
    
    // Change notifications
    void on_change(const std::string& key, ConfigChangeCallback callback) override;
    void remove_change_callback(const std::string& key) override;
    
    // Configuration persistence
    bool save_to_file(const std::string& filepath) const override;
    bool load_from_file(const std::string& filepath) override;
    
    // Configuration introspection
    std::vector<std::string> get_keys() const override;
    ConfigType get_type(const std::string& key) const override;
    std::string get_description(const std::string& key) const override;
    bool is_required(const std::string& key) const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ConfigItem> config_items_;
    std::unordered_map<std::string, ConfigChangeCallback> change_callbacks_;
    
    // Helper methods
    ValidationResult validate_value(const std::string& key, const std::any& value) const;
    void notify_change(const std::string& key, const std::any& old_value, const std::any& new_value);
    std::string serialize_value(const std::any& value, ConfigType type) const;
    std::any deserialize_value(const std::string& value_str, ConfigType type) const;
};

} // namespace config
} // namespace unilink
