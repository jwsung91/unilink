#pragma once
#include <memory>
#include <string>
#include <variant>

#include "ichannel.hpp"
#include "serial_config.hpp"

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
	using ChannelOptions = std::variant<TcpClientOptions, TcpServerSingleOptions, SerialOptions>;

	// Backward-compatible specific factories
	static std::shared_ptr<IChannel> make_tcp_client(class boost::asio::io_context& ioc,
	                                          const std::string& host,
	                                          uint16_t port);
	static std::shared_ptr<IChannel> make_tcp_server_single(
	    class boost::asio::io_context& ioc, uint16_t port);

	static std::shared_ptr<IChannel> make_serial_channel(
	    class boost::asio::io_context& ioc,
	    const std::string& device,  // 예: "/dev/ttyUSB0"
	    const SerialConfig& cfg);

	// Unified factory API
	static std::shared_ptr<IChannel> create(class boost::asio::io_context& ioc,
	                                     const ChannelOptions& options);
};
