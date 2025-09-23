#pragma once

#include <memory>
#include <variant>

#include "unilink/export.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {

// Public Interface
using Channel = interface::Channel;
using LinkState = common::LinkState;

// Configs
using TcpClientConfig = config::TcpClientConfig;
using TcpServerConfig = config::TcpServerConfig;
using SerialConfig = config::SerialConfig;

// Factory function
using ChannelOptions =
    std::variant<TcpClientConfig, TcpServerConfig, SerialConfig>;

// Common helpers
using common::feed_lines;
using common::log_message;
using common::to_cstr;
using common::ts_now;

UNILINK_API std::shared_ptr<Channel> create(const ChannelOptions& options) {
  return factory::ChannelFactory::create(options);
}

}  // namespace unilink