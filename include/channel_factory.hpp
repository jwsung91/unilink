#pragma once
#include <memory>
#include <string>
#include <variant>

#include "ichannel.hpp"
#include "serial.hpp"
#include "tcp_client.hpp"
#include "tcp_server.hpp"

class ChannelFactory {
 public:
  using ChannelOptions =
      std::variant<TcpClientConfig, TcpServerConfig, SerialOptions>;

  // Unified factory API
  static std::shared_ptr<IChannel> create(class boost::asio::io_context& ioc,
                                          const ChannelOptions& options);
};
