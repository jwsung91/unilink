#include <boost/asio.hpp>
#include <variant>
#include <string>

#include "factory.hpp"

// Forward declarations of free creators implemented in src/*.cc
std::shared_ptr<IChannel> make_tcp_client_impl(boost::asio::io_context& ioc,
                                               const std::string& host,
                                               uint16_t port);
std::shared_ptr<IChannel> make_tcp_server_single_impl(boost::asio::io_context& ioc,
                                                      uint16_t port);
std::shared_ptr<IChannel> make_serial_channel_impl(boost::asio::io_context& ioc,
                                                   const std::string& device,
                                                   const SerialConfig& cfg);

using ChannelOptions = ChannelFactory::ChannelOptions;

std::shared_ptr<IChannel> ChannelFactory::create(boost::asio::io_context& ioc,
                                               const ChannelOptions& options) {
	return std::visit(
	    [&](const auto& opt) -> std::shared_ptr<IChannel> {
	      using T = std::decay_t<decltype(opt)>;
	      if constexpr (std::is_same_v<T, TcpClientOptions>) {
	        return make_tcp_client_impl(ioc, opt.host, opt.port);
	      } else if constexpr (std::is_same_v<T, TcpServerSingleOptions>) {
	        return make_tcp_server_single_impl(ioc, opt.port);
	      } else if constexpr (std::is_same_v<T, SerialOptions>) {
	        return make_serial_channel_impl(ioc, opt.device, opt.cfg);
	      } else {
	        static_assert(!sizeof(T*), "Non-exhaustive visitor for ChannelOptions");
	      }
	    },
	    options);
}


