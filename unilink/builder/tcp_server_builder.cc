#include "unilink/builder/tcp_server_builder.hpp"

namespace unilink {
namespace builder {

TcpServerBuilder::TcpServerBuilder(uint16_t port)
    : port_(port), auto_start_(false), auto_manage_(false) {}

std::unique_ptr<wrapper::TcpServer> TcpServerBuilder::build() {
    auto server = std::make_unique<wrapper::TcpServer>(port_);
    
    // Apply configuration
    if (auto_start_) {
        server->auto_start(true);
    }
    
    if (auto_manage_) {
        server->auto_manage(true);
    }
    
    // Set callbacks
    if (on_data_) {
        server->on_data(on_data_);
    }
    
    if (on_connect_) {
        server->on_connect(on_connect_);
    }
    
    if (on_disconnect_) {
        server->on_disconnect(on_disconnect_);
    }
    
    if (on_error_) {
        server->on_error(on_error_);
    }
    
    return server;
}

TcpServerBuilder& TcpServerBuilder::auto_start(bool auto_start) {
    auto_start_ = auto_start;
    return *this;
}

TcpServerBuilder& TcpServerBuilder::auto_manage(bool auto_manage) {
    auto_manage_ = auto_manage;
    return *this;
}

TcpServerBuilder& TcpServerBuilder::on_data(std::function<void(const std::string&)> handler) {
    on_data_ = std::move(handler);
    return *this;
}

TcpServerBuilder& TcpServerBuilder::on_connect(std::function<void()> handler) {
    on_connect_ = std::move(handler);
    return *this;
}

TcpServerBuilder& TcpServerBuilder::on_disconnect(std::function<void()> handler) {
    on_disconnect_ = std::move(handler);
    return *this;
}

TcpServerBuilder& TcpServerBuilder::on_error(std::function<void(const std::string&)> handler) {
    on_error_ = std::move(handler);
    return *this;
}

} // namespace builder
} // namespace unilink

