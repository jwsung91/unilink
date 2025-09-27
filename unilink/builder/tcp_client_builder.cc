#include "unilink/builder/tcp_client_builder.hpp"

#include "unilink/wrapper/tcp_client/tcp_client.hpp"

namespace unilink {
namespace builder {

TcpClientBuilder::TcpClientBuilder(const std::string& host, uint16_t port) 
    : host_(host), port_(port) {}

IBuilder<wrapper::TcpClient>& TcpClientBuilder::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpClient>& TcpClientBuilder::on_connect(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpClient>& TcpClientBuilder::on_disconnect(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpClient>& TcpClientBuilder::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::TcpClient>& TcpClientBuilder::auto_start(bool start) {
    auto_start_ = start;
    return *this;
}

IBuilder<wrapper::TcpClient>& TcpClientBuilder::auto_manage(bool manage) {
    auto_manage_ = manage;
    return *this;
}

std::unique_ptr<wrapper::TcpClient> TcpClientBuilder::build() {
    auto client = std::make_unique<wrapper::TcpClient>(host_, port_);
    
    // 설정된 핸들러들 적용
    if (data_handler_) client->on_data(data_handler_);
    if (connect_handler_) client->on_connect(connect_handler_);
    if (disconnect_handler_) client->on_disconnect(disconnect_handler_);
    if (error_handler_) client->on_error(error_handler_);
    
    // 설정 적용
    if (auto_start_) client->auto_start();
    if (auto_manage_) client->auto_manage();
    
    // TCP 클라이언트 전용 설정 적용
    client->set_retry_interval(retry_interval_);
    client->set_max_retries(max_retries_);
    client->set_connection_timeout(connection_timeout_);
    
    return client;
}

TcpClientBuilder& TcpClientBuilder::host(const std::string& host) {
    host_ = host;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::port(uint16_t port) {
    port_ = port;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::retry_interval(std::chrono::milliseconds interval) {
    retry_interval_ = interval;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::max_retries(int max_retries) {
    max_retries_ = max_retries;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::connection_timeout(std::chrono::milliseconds timeout) {
    connection_timeout_ = timeout;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::keep_alive(bool enable) {
    keep_alive_ = enable;
    return *this;
}

} // namespace builder
} // namespace unilink
