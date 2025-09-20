#pragma once
#include <memory>
#include <string>
#include <variant>

#include "interface/ichannel.hpp"
#include "transport/serial/serial.hpp"
#include "transport/tcp/tcp_client.hpp"
#include "transport/tcp/tcp_server.hpp"

namespace unilink {
namespace factory {

using namespace interface;
using namespace config;

class ChannelFactory {
 public:
  using ChannelOptions =
      std::variant<TcpClientConfig, TcpServerConfig, SerialConfig>;

  // Unified factory API
  static std::shared_ptr<IChannel> create(class boost::asio::io_context& ioc,
                                          const ChannelOptions& options);
};
}  // namespace factory
}  // namespace unilink