#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/serial/serial.hpp"

namespace unilink {
namespace factory {

std::shared_ptr<interface::Channel> ChannelFactory::create(const ChannelOptions& options) {
    return std::visit([](const auto& config) -> std::shared_ptr<interface::Channel> {
        using T = std::decay_t<decltype(config)>;
        
        if constexpr (std::is_same_v<T, config::TcpClientConfig>) {
            return create_tcp_client(config);
        } else if constexpr (std::is_same_v<T, config::TcpServerConfig>) {
            return create_tcp_server(config);
        } else if constexpr (std::is_same_v<T, config::SerialConfig>) {
            return create_serial(config);
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported config type");
            return nullptr;
        }
    }, options);
}

std::shared_ptr<interface::Channel> ChannelFactory::create_tcp_server(const config::TcpServerConfig& cfg) {
    return std::make_shared<transport::TcpServer>(cfg);
}

std::shared_ptr<interface::Channel> ChannelFactory::create_tcp_client(const config::TcpClientConfig& cfg) {
    return std::make_shared<transport::TcpClient>(cfg);
}

std::shared_ptr<interface::Channel> ChannelFactory::create_serial(const config::SerialConfig& cfg) {
    return std::make_shared<transport::Serial>(cfg);
}

} // namespace factory
} // namespace unilink
