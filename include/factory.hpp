#pragma once
#include <memory>
#include <string>

#include "ichannel.hpp"
#include "serial_config.hpp"

std::shared_ptr<IChannel> make_tcp_client(class boost::asio::io_context& ioc,
                                          const std::string& host,
                                          uint16_t port);
std::shared_ptr<IChannel> make_tcp_server_single(
    class boost::asio::io_context& ioc, uint16_t port);

std::shared_ptr<IChannel> make_serial_channel(
    class boost::asio::io_context& ioc,
    const std::string& device,  // 예: "/dev/ttyUSB0"
    const SerialConfig& cfg);
