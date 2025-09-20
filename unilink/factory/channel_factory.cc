#include "factory/channel_factory.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <variant>

namespace unilink {
namespace factory {

using ChannelOptions = ChannelFactory::ChannelOptions;
using namespace interface;
using namespace transport;

std::shared_ptr<IChannel> ChannelFactory::create(const ChannelOptions& options) {
  return std::visit(
      [&](const auto& opt) -> std::shared_ptr<IChannel> {
        using T = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<T, TcpClientConfig>) {
          return std::make_shared<TcpClient>(opt);
        } else if constexpr (std::is_same_v<T, TcpServerConfig>) {
          return std::make_shared<TcpServer>(opt);
        } else if constexpr (std::is_same_v<T, SerialConfig>) {
          return std::make_shared<Serial>(opt);
        } else {
          static_assert(!sizeof(T*),
                        "Non-exhaustive visitor for ChannelOptions");
        }
      },
      options);
}

}  // namespace factory
}  // namespace unilink