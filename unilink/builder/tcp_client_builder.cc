#include "unilink/builder/tcp_client_builder.hpp"

namespace unilink {
namespace builder {

TcpClientBuilder::TcpClientBuilder(const std::string& host, uint16_t port)
    : host_(host), port_(port), auto_start_(false), auto_manage_(false) {}

std::unique_ptr<wrapper::TcpClient> TcpClientBuilder::build() {
    auto client = std::make_unique<wrapper::TcpClient>(host_, port_);
    
    // Apply configuration
    if (auto_start_) {
        client->auto_start(true);
    }
    
    if (auto_manage_) {
        client->auto_manage(true);
    }
    
    // Set callbacks
    if (on_data_) {
        client->on_data(on_data_);
    }
    
    if (on_connect_) {
        client->on_connect(on_connect_);
    }
    
    if (on_disconnect_) {
        client->on_disconnect(on_disconnect_);
    }
    
    if (on_error_) {
        client->on_error(on_error_);
    }
    
    return client;
}

TcpClientBuilder& TcpClientBuilder::auto_start(bool auto_start) {
    auto_start_ = auto_start;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::auto_manage(bool auto_manage) {
    auto_manage_ = auto_manage;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_data(std::function<void(const std::string&)> handler) {
    on_data_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_connect(std::function<void()> handler) {
    on_connect_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_disconnect(std::function<void()> handler) {
    on_disconnect_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_error(std::function<void(const std::string&)> handler) {
    on_error_ = std::move(handler);
    return *this;
}

} // namespace builder
} // namespace unilink

