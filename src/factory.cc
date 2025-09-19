#include <boost/asio.hpp>
#include <variant>

#include "factory.hpp"

using ChannelOptions = ChannelFactory::ChannelOptions;

std::shared_ptr<IChannel> ChannelFactory::create(boost::asio::io_context& ioc,
                                               const ChannelOptions& options) {
	return std::visit(
	    [&](const auto& opt) -> std::shared_ptr<IChannel> {
	      using T = std::decay_t<decltype(opt)>;
	      if constexpr (std::is_same_v<T, TcpClientOptions>) {
	        return ChannelFactory::make_tcp_client(ioc, opt.host, opt.port);
	      } else if constexpr (std::is_same_v<T, TcpServerSingleOptions>) {
	        return ChannelFactory::make_tcp_server_single(ioc, opt.port);
	      } else if constexpr (std::is_same_v<T, SerialOptions>) {
	        return ChannelFactory::make_serial_channel(ioc, opt.device, opt.cfg);
	      } else {
	        static_assert(!sizeof(T*), "Non-exhaustive visitor for ChannelOptions");
	      }
	    },
	    options);
}


