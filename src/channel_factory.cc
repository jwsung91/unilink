#include "channel_factory.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <variant>

using ChannelOptions = ChannelFactory::ChannelOptions;

std::shared_ptr<IChannel> ChannelFactory::create(
    boost::asio::io_context& ioc, const ChannelOptions& options) {
  return std::visit(
      [&](const auto& opt) -> std::shared_ptr<IChannel> {
        using T = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<T, TcpClientConfig>) {
          return std::make_shared<TcpClient>(ioc, opt);
        } else if constexpr (std::is_same_v<T, TcpServerConfig>) {
          return std::make_shared<TcpServer>(ioc, opt);
        } else if constexpr (std::is_same_v<T, SerialConfig>) {
          return std::make_shared<Serial>(ioc, opt);
        } else {
          static_assert(!sizeof(T*),
                        "Non-exhaustive visitor for ChannelOptions");
        }
      },
      options);
}
