#include "unilink/wrapper/tcp_server.hpp"

#include <iostream>
#include <chrono>

#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"

namespace unilink {
namespace wrapper {

TcpServer::TcpServer(uint16_t port) 
    : port_(port), channel_(nullptr) {
    // Channel은 나중에 start() 시점에 생성
}

TcpServer::TcpServer(std::shared_ptr<interface::Channel> channel)
    : port_(0), channel_(channel) {
    setup_internal_handlers();
}

TcpServer::~TcpServer() {
    if (auto_manage_ && started_) {
        stop();
    }
}

void TcpServer::start() {
    if (started_) return;
    
    if (!channel_) {
        // Channel 생성
        config::TcpServerConfig config{port_};
        channel_ = factory::ChannelFactory::create(config);
        setup_internal_handlers();
    }
    
    channel_->start();
    started_ = true;
    
    if (auto_start_) {
        // 이미 시작됨
    }
}

void TcpServer::stop() {
    if (!started_ || !channel_) return;
    
    channel_->stop();
    started_ = false;
}

void TcpServer::send(const std::string& data) {
    if (is_connected() && channel_) {
        channel_->async_write_copy(
            reinterpret_cast<const uint8_t*>(data.c_str()), 
            data.size()
        );
    }
}

void TcpServer::send_line(const std::string& line) {
    send(line + "\n");
}

bool TcpServer::is_connected() const {
    return channel_ && channel_->is_connected();
}

IChannel& TcpServer::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    if (channel_) {
        setup_internal_handlers();
    }
    return *this;
}

IChannel& TcpServer::on_connect(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
    return *this;
}

IChannel& TcpServer::on_disconnect(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
    return *this;
}

IChannel& TcpServer::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    return *this;
}

IChannel& TcpServer::auto_start(bool start) {
    auto_start_ = start;
    if (start && !started_) {
        this->start();
    }
    return *this;
}

IChannel& TcpServer::auto_manage(bool manage) {
    auto_manage_ = manage;
    if (manage && !started_) {
        auto_start(true);
    }
    return *this;
}

void TcpServer::set_max_connections(size_t max_connections) {
    max_connections_ = max_connections;
}

void TcpServer::set_timeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
}

void TcpServer::setup_internal_handlers() {
    if (!channel_) return;
    
    // 바이트 데이터를 문자열로 변환하여 전달
    channel_->on_bytes([this](const uint8_t* p, size_t n) {
        if (data_handler_) {
            std::string data(reinterpret_cast<const char*>(p), n);
            data_handler_(data);
        }
    });
    
    // 상태 변화 처리
    channel_->on_state([this](common::LinkState state) {
        notify_state_change(state);
    });
}

void TcpServer::notify_state_change(common::LinkState state) {
    switch (state) {
        case common::LinkState::Connected:
            if (connect_handler_) connect_handler_();
            break;
        case common::LinkState::Closed:
            if (disconnect_handler_) disconnect_handler_();
            break;
        case common::LinkState::Error:
            if (error_handler_) error_handler_("Connection error occurred");
            break;
        default:
            break;
    }
}

} // namespace wrapper
} // namespace unilink
