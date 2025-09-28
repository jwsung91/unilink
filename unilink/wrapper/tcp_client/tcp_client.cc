#include "unilink/wrapper/tcp_client/tcp_client.hpp"

#include <iostream>
#include <chrono>
#include <thread>

#include "unilink/config/tcp_client_config.hpp"
#include "unilink/factory/channel_factory.hpp"

namespace unilink {
namespace wrapper {

TcpClient::TcpClient(const std::string& host, uint16_t port)
    : host_(host), port_(port), channel_(nullptr) {
    // Channel은 나중에 start() 시점에 생성
}

TcpClient::TcpClient(std::shared_ptr<interface::Channel> channel)
    : host_(""), port_(0), channel_(channel) {
    setup_internal_handlers();
}

TcpClient::~TcpClient() {
    // 강제로 정리 - auto_manage 설정과 관계없이
    if (started_) {
        stop();
    }
    // Channel 리소스 명시적 정리
    if (channel_) {
        channel_.reset();
    }
}

void TcpClient::start() {
    if (started_) return;
    
    if (!channel_) {
        // Channel 생성
        config::TcpClientConfig config;
        config.host = host_;
        config.port = port_;
        config.retry_interval_ms = retry_interval_.count();
        channel_ = factory::ChannelFactory::create(config);
        setup_internal_handlers();
    }
    
    channel_->start();
    started_ = true;
    
    if (auto_start_) {
        // 이미 시작됨
    }
}

void TcpClient::stop() {
    if (!started_ || !channel_) return;
    
    channel_->stop();
    // 잠시 대기하여 비동기 작업 완료
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    channel_.reset();
    started_ = false;
}

void TcpClient::send(const std::string& data) {
    if (is_connected() && channel_) {
        channel_->async_write_copy(
            reinterpret_cast<const uint8_t*>(data.c_str()), 
            data.size()
        );
    }
}

void TcpClient::send_line(const std::string& line) {
    send(line + "\n");
}

bool TcpClient::is_connected() const {
    return channel_ && channel_->is_connected();
}

IChannel& TcpClient::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    if (channel_) {
        setup_internal_handlers();
    }
    return *this;
}

IChannel& TcpClient::on_connect(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
    return *this;
}

IChannel& TcpClient::on_disconnect(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
    return *this;
}

IChannel& TcpClient::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    return *this;
}

IChannel& TcpClient::auto_start(bool start) {
    auto_start_ = start;
    if (start && !started_) {
        this->start();
    }
    return *this;
}

IChannel& TcpClient::auto_manage(bool manage) {
    auto_manage_ = manage;
    if (manage && !started_) {
        auto_start(true);
    }
    return *this;
}

void TcpClient::set_retry_interval(std::chrono::milliseconds interval) {
    retry_interval_ = interval;
}

void TcpClient::set_max_retries(int max_retries) {
    max_retries_ = max_retries;
}

void TcpClient::set_connection_timeout(std::chrono::milliseconds timeout) {
    connection_timeout_ = timeout;
}

void TcpClient::setup_internal_handlers() {
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

void TcpClient::notify_state_change(common::LinkState state) {
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
