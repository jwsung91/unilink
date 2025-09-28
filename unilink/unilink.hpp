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