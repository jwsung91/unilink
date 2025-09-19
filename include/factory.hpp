#pragma once
#include <memory>
#include <string>
#include <variant>

#include "ichannel.hpp"
#include "serial.hpp"
#include "serial_config.hpp"
#include "tcp_client.hpp"
#include "tcp_server.hpp"

class ChannelFactory {
 public:
  // Option types for unified factory
  struct TcpClientOptions {
    std::string host;
    uint16_t port;
  };
  struct TcpServerSingleOptions {
    uint16_t port;
  };
  struct SerialOptions {
    std::string device;  // 예: "/dev/ttyUSB0"
    SerialConfig cfg;
  };
  using ChannelOptions =
      std::variant<TcpClientOptions, TcpServerSingleOptions, SerialOptions>;

  // Unified factory API
  static std::shared_ptr<IChannel> create(class boost::asio::io_context& ioc,
                                          const ChannelOptions& options);
};
