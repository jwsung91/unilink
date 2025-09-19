#include "channel_factory.hpp"

#include <boost/asio.hpp>
#include <string>
#include <variant>

using ChannelOptions = ChannelFactory::ChannelOptions;

std::shared_ptr<IChannel> ChannelFactory::create(
    boost::asio::io_context& ioc, const ChannelOptions& options) {
  return std::visit(
      [&](const auto& opt) -> std::shared_ptr<IChannel> {
        using T = std::decay_t<decltype(opt)>;
        if constexpr (std::is_same_v<T, TcpClientOptions>) {
          return std::make_shared<TcpClient>(ioc, opt.host, opt.port);
        } else if constexpr (std::is_same_v<T, TcpServerSingleOptions>) {
          return std::make_shared<TcpServer>(ioc, opt.port);
        } else if constexpr (std::is_same_v<T, SerialOptions>) {
          return std::make_shared<Serial>(ioc, opt.device, opt.cfg);
        } else {
          static_assert(!sizeof(T*),
                        "Non-exhaustive visitor for ChannelOptions");
        }
      },
      options);
}
