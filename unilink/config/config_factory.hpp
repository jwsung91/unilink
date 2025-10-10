#pragma once

#include <memory>
#include <mutex>

#include "iconfig_manager.hpp"

namespace unilink {
namespace config {

/**
 * Factory for creating configuration managers
 */
class ConfigFactory {
 public:
  /**
   * Create a new configuration manager instance
   */
  static std::shared_ptr<ConfigManagerInterface> create();

  /**
   * Create a configuration manager with default settings
   */
  static std::shared_ptr<ConfigManagerInterface> create_with_defaults();

  /**
   * Create a configuration manager and load from file
   */
  static std::shared_ptr<ConfigManagerInterface> create_from_file(const std::string& filepath);

  /**
   * Create a singleton configuration manager
   */
  static std::shared_ptr<ConfigManagerInterface> get_singleton();

 private:
  static std::shared_ptr<ConfigManagerInterface> singleton_instance_;
  static std::mutex singleton_mutex_;
};

/**
 * Configuration presets for common use cases
 */
class ConfigPresets {
 public:
  /**
   * Setup default configuration for TCP client
   */
  static void setup_tcp_client_defaults(std::shared_ptr<ConfigManagerInterface> config);

  /**
   * Setup default configuration for TCP server
   */
  static void setup_tcp_server_defaults(std::shared_ptr<ConfigManagerInterface> config);

  /**
   * Setup default configuration for Serial communication
   */
  static void setup_serial_defaults(std::shared_ptr<ConfigManagerInterface> config);

  /**
   * Setup default configuration for logging
   */
  static void setup_logging_defaults(std::shared_ptr<ConfigManagerInterface> config);

  /**
   * Setup default configuration for all components
   */
  static void setup_all_defaults(std::shared_ptr<ConfigManagerInterface> config);
};

}  // namespace config
}  // namespace unilink
