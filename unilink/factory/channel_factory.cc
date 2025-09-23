#include "unilink/factory/channel_factory.hpp"

namespace unilink {
namespace factory {

using ChannelOptions = ChannelFactory::ChannelOptions;
using namespace interface;
using namespace transport;
using namespace config;

std::shared_ptr<Channel> ChannelFactory::create(const ChannelOptions& options) {
  return std::visit(
      [&](const auto& opt) -> std::shared_ptr<Channel> {
        using T = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<T, TcpClientConfig>) {
          return std::make_shared<TcpClient>(opt);
        } else if constexpr (std::is_same_v<T, TcpServerConfig>) {
          return std::make_shared<TcpServer>(opt);
        } else if constexpr (std::is_same_v<T, SerialConfig>) {
          return std::make_shared<Serial>(opt);
        } else {
          static_assert(std::false_type::value,
                        "Non-exhaustive visitor for ChannelOptions");
        }
      },
      options);
}

}  // namespace factory
}  // namespace unilink