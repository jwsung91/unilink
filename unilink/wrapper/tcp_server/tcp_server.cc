#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include <iostream>

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

void TcpServer::start() {
    if (started_) return;
    
    if (!channel_) {
        // 개선된 Factory 사용
        config::TcpServerConfig config;
        config.port = port_;
        channel_ = factory::ChannelFactory::create(config);
        setup_internal_handlers();
    }
    
    channel_->start();
    started_ = true;
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

bool TcpServer::is_connected() const {
    return channel_ && channel_->is_connected();
}

IChannel& TcpServer::on_data(DataHandler handler) {
    on_data_ = std::move(handler);
    return *this;
}

IChannel& TcpServer::on_connect(ConnectHandler handler) {
    on_connect_ = std::move(handler);
    return *this;
}

IChannel& TcpServer::on_disconnect(DisconnectHandler handler) {
    on_disconnect_ = std::move(handler);
    return *this;
}

IChannel& TcpServer::on_error(ErrorHandler handler) {
    on_error_ = std::move(handler);
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
    return *this;
}

void TcpServer::send_line(const std::string& line) {
    send(line + "\n");
}

// void ImprovedTcpServer::send_binary(const std::vector<uint8_t>& data) {
//     if (is_connected() && channel_) {
//         channel_->async_write_copy(data.data(), data.size());
//     }
// }

void TcpServer::setup_internal_handlers() {
    if (!channel_) return;
    
    channel_->on_bytes([this](const uint8_t* data, size_t size) {
        handle_bytes(data, size);
    });
    
    channel_->on_state([this](common::LinkState state) {
        handle_state(state);
    });
}

void TcpServer::handle_bytes(const uint8_t* data, size_t size) {
    if (on_data_) {
        std::string str_data(reinterpret_cast<const char*>(data), size);
        on_data_(str_data);
    }
}

void TcpServer::handle_state(common::LinkState state) {
    switch (state) {
        case common::LinkState::Connected:
            if (on_connect_) {
                on_connect_();
            }
            break;
        case common::LinkState::Closed:
            if (on_disconnect_) {
                on_disconnect_();
            }
            break;
        case common::LinkState::Error:
            if (on_error_) {
                on_error_("Connection error");
            }
            break;
        default:
            break;
    }
}

} // namespace wrapper
} // namespace unilink
