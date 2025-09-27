#include "unilink/builder/tcp_server_builder.hpp"

#include "unilink/wrapper/tcp_server/tcp_server.hpp"

namespace unilink {
namespace builder {

TcpServerBuilder::TcpServerBuilder(uint16_t port) : port_(port) {}

IBuilder<wrapper::TcpServer>& TcpServerBuilder::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpServer>& TcpServerBuilder::on_connect(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpServer>& TcpServerBuilder::on_disconnect(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpServer>& TcpServerBuilder::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpServer>& TcpServerBuilder::auto_start(bool start) {
    auto_start_ = start;
    return *this;
}

IBuilder<wrapper::TcpServer>& TcpServerBuilder::auto_manage(bool manage) {
    auto_manage_ = manage;
    return *this;
}

std::unique_ptr<wrapper::TcpServer> TcpServerBuilder::build() {
    auto server = std::make_unique<wrapper::TcpServer>(port_);
    
    // 설정된 핸들러들 적용
    if (data_handler_) server->on_data(data_handler_);
    if (connect_handler_) server->on_connect(connect_handler_);
    if (disconnect_handler_) server->on_disconnect(disconnect_handler_);
    if (error_handler_) server->on_error(error_handler_);
    
    // 설정 적용
    if (auto_start_) server->auto_start();
    if (auto_manage_) server->auto_manage();
    
    // TCP 서버 전용 설정 적용
    server->set_max_connections(max_connections_);
    server->set_timeout(timeout_);
    
    return server;
}

TcpServerBuilder& TcpServerBuilder::port(uint16_t port) {
    port_ = port;
    return *this;
}

TcpServerBuilder& TcpServerBuilder::max_connections(size_t max_connections) {
    max_connections_ = max_connections;
    return *this;
}

TcpServerBuilder& TcpServerBuilder::timeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
    return *this;
}

TcpServerBuilder& TcpServerBuilder::buffer_size(size_t buffer_size) {
    buffer_size_ = buffer_size;
    return *this;
}

} // namespace builder
} // namespace unilink
