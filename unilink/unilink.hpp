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

// 새로운 고수준 API includes
#include "unilink/wrapper/ichannel.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"
#include "unilink/wrapper/serial/serial.hpp"
#include "unilink/builder/unified_builder.hpp"

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

// === 새로운 고수준 API (Builder 패턴) ===
// 간단한 팩토리 함수들 - Builder 반환
inline auto tcp_server(uint16_t port) {
    return builder::UnifiedBuilder::tcp_server(port);
}

inline auto tcp_client(const std::string& host, uint16_t port) {
    return builder::UnifiedBuilder::tcp_client(host, port);
}

inline auto serial(const std::string& device, uint32_t baud_rate = 9600) {
    return builder::UnifiedBuilder::serial(device, baud_rate);
}

// === 편의 별칭들 ===
namespace wrapper {
    using TcpServer = wrapper::TcpServer;
    using TcpClient = wrapper::TcpClient;
    using Serial = wrapper::Serial;
    using IChannel = wrapper::IChannel;
}

namespace builder {
    using TcpServerBuilder = builder::TcpServerBuilder;
    using TcpClientBuilder = builder::TcpClientBuilder;
    using SerialBuilder = builder::SerialBuilder;
    using UnifiedBuilder = builder::UnifiedBuilder;
}

// === 공통 유틸리티 (기존 유지) ===
using common::feed_lines;
using common::log_message;
using common::to_cstr;
using common::ts_now;

} // namespace unilink