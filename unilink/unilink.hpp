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

// === Public API Namespaces ===
// Clean namespace structure without redundant aliases

// Builder API namespace
namespace builder {
// Forward declarations for cleaner includes
class TcpServerBuilder;
class TcpClientBuilder;
class SerialBuilder;
class UnifiedBuilder;
}  // namespace builder

// Wrapper API namespace
namespace wrapper {
// Forward declarations for cleaner includes
class TcpServer;
class TcpClient;
class Serial;
class ChannelInterface;
}  // namespace wrapper

// === Public Builder API Convenience Functions ===
// Convenience functions to make Builder pattern easier to use

// === Convenience Functions ===
// Simplified API for common use cases

/**
 * @brief Create a TCP server builder
 * @param port The port number for the server
 * @return TcpServerBuilder A configured builder for TcpServer
 */
inline builder::TcpServerBuilder tcp_server(uint16_t port) { return builder::TcpServerBuilder(port); }

/**
 * @brief Create a TCP client builder
 * @param host The host address to connect to
 * @param port The port number to connect to
 * @return TcpClientBuilder A configured builder for TcpClient
 */
inline builder::TcpClientBuilder tcp_client(const std::string& host, uint16_t port) {
  return builder::TcpClientBuilder(host, port);
}

/**
 * @brief Create a Serial port builder
 * @param device The serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate The baud rate for serial communication
 * @return SerialBuilder A configured builder for Serial
 */
inline builder::SerialBuilder serial(const std::string& device, uint32_t baud_rate) {
  return builder::SerialBuilder(device, baud_rate);
}

// === Configuration Management API (optional) ===
#ifdef UNILINK_ENABLE_CONFIG
namespace config {
// Forward declarations for configuration management
class ConfigManager;
class ConfigFactory;
class ConfigManagerInterface;
}  // namespace config
#endif

// === Common Utilities ===
// Utility functions and error handling
namespace common {
// Utility functions
using common::feed_lines;
using common::log_message;
using common::to_cstr;
using common::ts_now;

// Error handling and logging
using common::ErrorCategory;
using common::ErrorHandler;
using common::ErrorInfo;
using common::ErrorLevel;
using common::Logger;
using common::LogLevel;
}  // namespace common

}  // namespace unilink