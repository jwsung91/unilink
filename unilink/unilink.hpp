#pragma once

#include <memory>
#include <variant>

// Internal includes for Builder API (not exposed to users)
#include "unilink/wrapper/ichannel.hpp"
#include "unilink/wrapper/serial/serial.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"

// New high-level Builder API includes
#include "unilink/builder/ibuilder.hpp"
#include "unilink/builder/serial_builder.hpp"
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/tcp_server_builder.hpp"
#include "unilink/builder/unified_builder.hpp"

// Configuration Management API includes (optional)
#ifdef UNILINK_ENABLE_CONFIG
#include "unilink/config/config_factory.hpp"
#include "unilink/config/config_manager.hpp"
#include "unilink/config/iconfig_manager.hpp"
#endif

// Error handling and logging system includes
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"

namespace unilink {

// === Internal Wrapper API for Builder API ===
// Convenience aliases (for internal use only)
namespace wrapper {
using TcpServer = wrapper::TcpServer;
using TcpClient = wrapper::TcpClient;
using Serial = wrapper::Serial;
using IChannel = wrapper::ChannelInterface;
}  // namespace wrapper

// === Public Builder API ===
// Builder APIs available to users
namespace builder {
using TcpServerBuilder = builder::TcpServerBuilder;
using TcpClientBuilder = builder::TcpClientBuilder;
using SerialBuilder = builder::SerialBuilder;
using UnifiedBuilder = builder::UnifiedBuilder;
}  // namespace builder

// === Public Builder API Convenience Functions ===
// Convenience functions to make Builder pattern easier to use

/**
 * @brief Create a TcpServer builder with simple configuration
 * @param port The port number for the server
 * @return builder::TcpServerBuilder A configured builder for TcpServer
 */
inline builder::TcpServerBuilder tcp_server_builder(uint16_t port) { return builder::TcpServerBuilder(port); }

/**
 * @brief Create a TcpClient builder with simple configuration
 * @param host The host address to connect to
 * @param port The port number to connect to
 * @return builder::TcpClientBuilder A configured builder for TcpClient
 */
inline builder::TcpClientBuilder tcp_client_builder(const std::string& host, uint16_t port) {
  return builder::TcpClientBuilder(host, port);
}

/**
 * @brief Create a Serial builder with simple configuration
 * @param device The serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate The baud rate for serial communication
 * @return builder::SerialBuilder A configured builder for Serial
 */
inline builder::SerialBuilder serial_builder(const std::string& device, uint32_t baud_rate) {
  return builder::SerialBuilder(device, baud_rate);
}

/**
 * @brief Create a TcpServer builder (shortest form)
 * @param port The port number for the server
 * @return builder::TcpServerBuilder A configured builder for TcpServer
 */
inline builder::TcpServerBuilder tcp_server(uint16_t port) { return builder::TcpServerBuilder(port); }

/**
 * @brief Create a TcpClient builder (shortest form)
 * @param host The host address to connect to
 * @param port The port number to connect to
 * @return builder::TcpClientBuilder A configured builder for TcpClient
 */
inline builder::TcpClientBuilder tcp_client(const std::string& host, uint16_t port) {
  return builder::TcpClientBuilder(host, port);
}

/**
 * @brief Create a Serial builder (shortest form)
 * @param device The serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate The baud rate for serial communication
 * @return builder::SerialBuilder A configured builder for Serial
 */
inline builder::SerialBuilder serial(const std::string& device, uint32_t baud_rate) {
  return builder::SerialBuilder(device, baud_rate);
}

// === Configuration Management API (optional) ===
// Convenience aliases - only available when Config is enabled
#ifdef UNILINK_ENABLE_CONFIG
namespace config_manager {
using IConfigManager = config::ConfigManagerInterface;
using ConfigManager = config::ConfigManager;
using ConfigFactory = config::ConfigFactory;
using ConfigPresets = config::ConfigPresets;
using ConfigType = config::ConfigType;
using ConfigItem = config::ConfigItem;
using ValidationResult = config::ValidationResult;
using ConfigChangeCallback = config::ConfigChangeCallback;
}  // namespace config_manager
#endif

// === Common Utilities ===
// Useful utility functions
using common::feed_lines;
using common::log_message;
using common::to_cstr;
using common::ts_now;

// === Error Handling and Logging System ===
// Using new error handling and logging system
using common::ErrorCategory;
using common::ErrorHandler;
using common::ErrorInfo;
using common::ErrorLevel;
using common::Logger;
using common::LogLevel;

}  // namespace unilink