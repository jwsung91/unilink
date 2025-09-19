#pragma once
#include <memory>
#include <string>
#include <variant>

#include "interface/ichannel.hpp"
#include "module/serial.hpp"
#include "module/tcp_client.hpp"
#include "module/tcp_server.hpp"

class ChannelFactory {
 public:
  using ChannelOptions =
      std::variant<TcpClientConfig, TcpServerConfig, SerialConfig>;

  // Unified factory API
  static std::shared_ptr<IChannel> create(class boost::asio::io_context& ioc,
                                          const ChannelOptions& options);
};
