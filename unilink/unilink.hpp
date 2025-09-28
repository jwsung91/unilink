#pragma once

#include <memory>
#include <variant>

// 기존 저수준 API includes
#include "unilink/export.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

// 새로운 고수준 Wrapper API includes
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

// 새로운 Configuration Management API includes
#include "unilink/config_manager/iconfig_manager.hpp"
#include "unilink/config_manager/config_manager.hpp"
#include "unilink/config_manager/config_factory.hpp"

namespace unilink {

// === 기존 저수준 API (하위 호환성 유지) ===
using Channel = interface::Channel;
using LinkState = common::LinkState;

// Configs
using TcpClientConfig = config::TcpClientConfig;
using TcpServerConfig = config::TcpServerConfig;
using SerialConfig = config::SerialConfig;

// Factory function
using ChannelOptions =
    std::variant<TcpClientConfig, TcpServerConfig, SerialConfig>;

// 기존 Factory 함수 (저수준 API)
UNILINK_API std::shared_ptr<Channel> create(const ChannelOptions& options) {
  return factory::ChannelFactory::create(options);
}

// === 새로운 고수준 Wrapper API ===
// 편의 별칭들
namespace wrapper {
    using TcpServer = wrapper::TcpServer;
    using TcpClient = wrapper::TcpClient;
    using Serial = wrapper::Serial;
    using IChannel = wrapper::IChannel;
}

// === 새로운 고수준 Builder API ===
// 편의 별칭들
namespace builder {
    using TcpServerBuilder = builder::TcpServerBuilder;
    using TcpClientBuilder = builder::TcpClientBuilder;
    using SerialBuilder = builder::SerialBuilder;
    using UnifiedBuilder = builder::UnifiedBuilder;
}

// === 간단한 팩토리 함수들 (3단계 요구사항) ===
// 고수준 API를 위한 편의 함수들

/**
 * @brief Create a TcpServer wrapper with simple configuration
 * @param port The port number for the server
 * @return std::unique_ptr<wrapper::TcpServer> A configured server instance
 */
inline std::unique_ptr<wrapper::TcpServer> tcp_server(uint16_t port) {
    return std::make_unique<wrapper::TcpServer>(port);
}

/**
 * @brief Create a TcpClient wrapper with simple configuration
 * @param host The host address to connect to
 * @param port The port number to connect to
 * @return std::unique_ptr<wrapper::TcpClient> A configured client instance
 */
inline std::unique_ptr<wrapper::TcpClient> tcp_client(const std::string& host, uint16_t port) {
    return std::make_unique<wrapper::TcpClient>(host, port);
}

/**
 * @brief Create a Serial wrapper with simple configuration
 * @param device The serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate The baud rate for serial communication
 * @return std::unique_ptr<wrapper::Serial> A configured serial instance
 */
inline std::unique_ptr<wrapper::Serial> serial(const std::string& device, uint32_t baud_rate) {
    return std::make_unique<wrapper::Serial>(device, baud_rate);
}

// === 간단한 Builder 함수들 (2단계 개선) ===
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

// === 더 간단한 Builder 별칭들 ===
// 가장 간단한 사용법을 위한 별칭들

/**
 * @brief Create a TcpServer builder (shortest form)
 * @param port The port number for the server
 * @return builder::TcpServerBuilder A configured builder for TcpServer
 */
inline builder::TcpServerBuilder server(uint16_t port) {
    return builder::TcpServerBuilder(port);
}

/**
 * @brief Create a TcpClient builder (shortest form)
 * @param host The host address to connect to
 * @param port The port number to connect to
 * @return builder::TcpClientBuilder A configured builder for TcpClient
 */
inline builder::TcpClientBuilder client(const std::string& host, uint16_t port) {
    return builder::TcpClientBuilder(host, port);
}

/**
 * @brief Create a Serial builder (shortest form)
 * @param device The serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate The baud rate for serial communication
 * @return builder::SerialBuilder A configured builder for Serial
 */
inline builder::SerialBuilder serial_port(const std::string& device, uint32_t baud_rate) {
    return builder::SerialBuilder(device, baud_rate);
}

// === 새로운 Configuration Management API ===
// 편의 별칭들
namespace config_manager {
    using IConfigManager = config_manager::IConfigManager;
    using ConfigManager = config_manager::ConfigManager;
    using ConfigFactory = config_manager::ConfigFactory;
    using ConfigPresets = config_manager::ConfigPresets;
    using ConfigType = config_manager::ConfigType;
    using ConfigItem = config_manager::ConfigItem;
    using ValidationResult = config_manager::ValidationResult;
    using ConfigChangeCallback = config_manager::ConfigChangeCallback;
}

// === 공통 유틸리티 (기존 유지) ===
using common::feed_lines;
using common::log_message;
using common::to_cstr;
using common::ts_now;

} // namespace unilink