#pragma once
#include <memory>
#include <string>
#include <variant>

#include "unilink/export.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {
namespace factory {

using namespace interface;
using namespace config;

class ChannelFactory {
 public:
  using ChannelOptions =
      std::variant<TcpClientConfig, TcpServerConfig, SerialConfig>;

  UNILINK_API static std::shared_ptr<Channel> create(
      const ChannelOptions& options);
};
}  // namespace factory
}  // namespace unilink