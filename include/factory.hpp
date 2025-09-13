#pragma once
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "ichannel.hpp"

std::shared_ptr<IChannel> make_client_single(boost::asio::io_context& ioc,
                                             const std::string& host,
                                             uint16_t port);

std::shared_ptr<IChannel> make_server_single(boost::asio::io_context& ioc,
                                             uint16_t port);
