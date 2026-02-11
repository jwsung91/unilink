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

#include "config_manager.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "unilink/diagnostics/logger.hpp"

namespace unilink {
namespace config {

std::any ConfigManager::get(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    return it->second.value;
  }
  throw std::runtime_error("Configuration key not found: " + key);
}

std::any ConfigManager::get(const std::string& key,
                            const std::any& default_value) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    return it->second.value;
  }
  return default_value;
}

bool ConfigManager::has(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return config_items_.find(key) != config_items_.end();
}

ValidationResult ConfigManager::set(const std::string& key,
                                    const std::any& value) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Validate the value
  auto validation_result = validate_value(key, value);
  if (!validation_result.is_valid) {
    return validation_result;
  }

  // Store old value for change notification
  std::any old_value;
  bool had_key = false;
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    old_value = it->second.value;
    had_key = true;
    it->second.value = value;
  } else {
    // Create new config item
    ConfigItem item(key, value, ConfigType::String, false);
    config_items_[key] = item;
  }

  // Notify change
  if (had_key) {
    notify_change(key, old_value, value);
  }

  return ValidationResult::success();
}

bool ConfigManager::remove(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    config_items_.erase(it);
    return true;
  }
  return false;
}

void ConfigManager::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  config_items_.clear();
}

ValidationResult ConfigManager::validate() const {
  std::lock_guard<std::mutex> lock(mutex_);

  for (const auto& [key, item] : config_items_) {
    auto result = validate_value(key, item.value);
    if (!result.is_valid) {
      return result;
    }
  }

  return ValidationResult::success();
}

ValidationResult ConfigManager::validate(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it == config_items_.end()) {
    return ValidationResult::error("Configuration key not found: " + key);
  }

  return validate_value(key, it->second.value);
}

void ConfigManager::register_item(const ConfigItem& item) {
  std::lock_guard<std::mutex> lock(mutex_);
  config_items_[item.key] = item;
}

void ConfigManager::register_validator(
    const std::string& key,
    std::function<ValidationResult(const std::any&)> validator) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    it->second.validator = validator;
  }
}

void ConfigManager::on_change(const std::string& key,
                              ConfigChangeCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  change_callbacks_[key] = callback;
}

void ConfigManager::remove_change_callback(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  change_callbacks_.erase(key);
}

bool ConfigManager::save_to_file(const std::string& filepath) const {
  std::lock_guard<std::mutex> lock(mutex_);

  try {
    std::ofstream file(filepath);
    if (!file.is_open()) {
      return false;
    }

    file << "# unilink configuration file\n";
    file << "# Generated automatically\n\n";

    for (const auto& [key, item] : config_items_) {
      file << "# " << item.description << "\n";
      file << key << "=" << serialize_value(item.value, item.type) << "\n\n";
    }

    return true;
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("config_manager", "save",
                      "Error saving configuration: " + std::string(e.what()));
    return false;
  }
}

bool ConfigManager::load_from_file(const std::string& filepath) {
  std::lock_guard<std::mutex> lock(mutex_);

  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      return false;
    }

    std::string line;
    while (std::getline(file, line)) {
      // Skip comments and empty lines
      if (line.empty() || line[0] == '#') {
        continue;
      }

      // Parse key=value pairs
      size_t pos = line.find('=');
      if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value_str = line.substr(pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value_str.erase(0, value_str.find_first_not_of(" \t"));
        value_str.erase(value_str.find_last_not_of(" \t") + 1);

        // Try to determine type
        ConfigType type = ConfigType::String;
        auto it = config_items_.find(key);
        bool exists = (it != config_items_.end());

        if (exists) {
          // Enforce existing type for security
          type = it->second.type;
        } else {
          // Infer type for new items
          if (value_str == "true" || value_str == "false") {
            type = ConfigType::Boolean;
          } else if (std::all_of(
                         value_str.begin(), value_str.end(), [](char c) {
                           return std::isdigit(static_cast<unsigned char>(c)) ||
                                  c == '-';
                         })) {
            type = ConfigType::Integer;
          } else if (std::count(value_str.begin(), value_str.end(), '.') == 1 &&
                     std::all_of(
                         value_str.begin(), value_str.end(), [](char c) {
                           return std::isdigit(static_cast<unsigned char>(c)) ||
                                  c == '.' || c == '-';
                         })) {
            type = ConfigType::Double;
          }
        }

        std::any value = deserialize_value(value_str, type);

        if (exists) {
          // Validate against existing constraints
          auto result = validate_value(key, value);
          if (!result.is_valid) {
            UNILINK_LOG_ERROR("config_manager", "load",
                              "Validation failed for key '" + key +
                                  "': " + result.error_message);
            continue;
          }

          // Update value while preserving metadata (validators, description,
          // etc.)
          std::any old_value = it->second.value;
          it->second.value = value;
          notify_change(key, old_value, value);
        } else {
          // Create new item
          ConfigItem item(key, value, type, false);
          config_items_[key] = item;
        }
      }
    }

    return true;
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("config_manager", "load",
                      "Error loading configuration: " + std::string(e.what()));
    return false;
  }
}

std::vector<std::string> ConfigManager::get_keys() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> keys;
  keys.reserve(config_items_.size());

  for (const auto& [key, item] : config_items_) {
    keys.push_back(key);
  }

  return keys;
}

ConfigType ConfigManager::get_type(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    return it->second.type;
  }
  throw std::runtime_error("Configuration key not found: " + key);
}

std::string ConfigManager::get_description(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    return it->second.description;
  }
  return "";
}

bool ConfigManager::is_required(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    return it->second.required;
  }
  return false;
}

ValidationResult ConfigManager::validate_value(const std::string& key,
                                               const std::any& value) const {
  auto it = config_items_.find(key);
  if (it != config_items_.end()) {
    // Check if there's a custom validator
    if (it->second.validator) {
      return it->second.validator(value);
    }

    // Basic type validation
    ConfigType expected_type = it->second.type;
    ConfigType actual_type = ConfigType::String;

    if (value.type() == typeid(std::string)) {
      actual_type = ConfigType::String;
    } else if (value.type() == typeid(int)) {
      actual_type = ConfigType::Integer;
    } else if (value.type() == typeid(bool)) {
      actual_type = ConfigType::Boolean;
    } else if (value.type() == typeid(double)) {
      actual_type = ConfigType::Double;
    }

    if (expected_type != actual_type) {
      return ValidationResult::error("Type mismatch for key '" + key + "'");
    }
  }

  return ValidationResult::success();
}

void ConfigManager::notify_change(const std::string& key,
                                  const std::any& old_value,
                                  const std::any& new_value) {
  auto it = change_callbacks_.find(key);
  if (it != change_callbacks_.end()) {
    try {
      it->second(key, old_value, new_value);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("config_manager", "callback",
                        "Error in change callback for key '" + key +
                            "': " + std::string(e.what()));
    }
  }
}

std::string ConfigManager::serialize_value(const std::any& value,
                                           ConfigType type) const {
  try {
    switch (type) {
      case ConfigType::String:
        return std::any_cast<std::string>(value);
      case ConfigType::Integer:
        return std::to_string(std::any_cast<int>(value));
      case ConfigType::Boolean:
        return std::any_cast<bool>(value) ? "true" : "false";
      case ConfigType::Double:
        return std::to_string(std::any_cast<double>(value));
      default:
        return "unknown";
    }
  } catch (const std::bad_any_cast&) {
    // Fallback to string representation
    return "unknown";
  }
}

std::any ConfigManager::deserialize_value(const std::string& value_str,
                                          ConfigType type) const {
  try {
    switch (type) {
      case ConfigType::String:
        return std::any(value_str);
      case ConfigType::Integer:
        return std::any(std::stoi(value_str));
      case ConfigType::Boolean:
        return std::any(value_str == "true");
      case ConfigType::Double:
        return std::any(std::stod(value_str));
      default:
        return std::any(value_str);
    }
  } catch (const std::exception&) {
    // Fallback to string if parsing fails
    return std::any(value_str);
  }
}

}  // namespace config
}  // namespace unilink
