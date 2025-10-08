#pragma once

#include <memory>
#include <variant>

// Builder API를 위한 내부 includes (사용자에게는 노출되지 않음)
#include "unilink/wrapper/ichannel.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"
#include "unilink/wrapper/serial/serial.hpp"

// 새로운 고수준 Builder API includes
#include "unilink/builder/ibuilder.hpp"
#include "unilink/builder/tcp_server_builder.hpp"
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/serial_builder.hpp"
#include "unilink/builder/unified_builder.hpp"

// Configuration Management API includes (optional)
#ifdef UNILINK_ENABLE_CONFIG
#include "unilink/config/iconfig_manager.hpp"
#include "unilink/config/config_manager.hpp"
#include "unilink/config/config_factory.hpp"
#endif

// Error handling and logging system includes
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"

namespace unilink {

// === Builder API를 위한 내부 Wrapper API ===
// 편의 별칭들 (내부적으로만 사용)
namespace wrapper {
    using TcpServer = wrapper::TcpServer;
    using TcpClient = wrapper::TcpClient;
    using Serial = wrapper::Serial;
    using IChannel = wrapper::ChannelInterface;
}

// === 공개 Builder API ===
// 사용자가 사용할 수 있는 Builder API들
namespace builder {
    using TcpServerBuilder = builder::TcpServerBuilder;
    using TcpClientBuilder = builder::TcpClientBuilder;
    using SerialBuilder = builder::SerialBuilder;
    using UnifiedBuilder = builder::UnifiedBuilder;
}

// === 공개 Builder API 편의 함수들 ===
// Builder 패턴을 더 간단하게 사용할 수 있는 편의 함수들

/**
 * @brief Create a TcpServer builder with simple configuration
 * @param port The port number for the server
 * @return builder::TcpServerBuilder A configured builder for TcpServer
 */
inline builder::TcpServerBuilder tcp_server_builder(uint16_t port) {
    return builder::TcpServerBuilder(port);
}

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
inline builder::TcpServerBuilder tcp_server(uint16_t port) {
    return builder::TcpServerBuilder(port);
}

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
// 편의 별칭들 - Config가 활성화된 경우에만 사용 가능
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
}
#endif

// === 공통 유틸리티 ===
// 유용한 유틸리티 함수들
using common::feed_lines;
using common::log_message;
using common::to_cstr;
using common::ts_now;

// === 에러 처리 및 로깅 시스템 ===
// 새로운 에러 처리 및 로깅 시스템 사용
using common::Logger;
using common::ErrorHandler;
using common::ErrorInfo;
using common::ErrorLevel;
using common::ErrorCategory;
using common::LogLevel;

} // namespace unilink