#include "unilink/wrapper/serial/serial.hpp"

#include <iostream>
#include <chrono>
#include <thread>

#include "unilink/config/serial_config.hpp"
#include "unilink/factory/channel_factory.hpp"

namespace unilink {
namespace wrapper {

Serial::Serial(const std::string& device, uint32_t baud_rate)
    : device_(device), baud_rate_(baud_rate), channel_(nullptr) {
    // Channel은 나중에 start() 시점에 생성
}

Serial::Serial(std::shared_ptr<interface::Channel> channel)
    : device_(""), baud_rate_(9600), channel_(channel) {
    setup_internal_handlers();
}

Serial::~Serial() {
    // 강제로 정리 - auto_manage 설정과 관계없이
    if (started_) {
        stop();
    }
    // Channel 리소스 명시적 정리
    if (channel_) {
        channel_.reset();
    }
}

void Serial::start() {
    if (started_) return;
    
    if (!channel_) {
        // Channel 생성
        config::SerialConfig config;
        config.device = device_;
        config.baud_rate = baud_rate_;
        config.char_size = data_bits_;
        config.stop_bits = stop_bits_;
        // parity와 flow는 enum으로 변환 필요
        config.flow = unilink::config::SerialConfig::Flow::None;
        channel_ = factory::ChannelFactory::create(config);
        setup_internal_handlers();
    }
    
    channel_->start();
    started_ = true;
    
    if (auto_start_) {
        // 이미 시작됨
    }
}

void Serial::stop() {
    if (!started_ || !channel_) return;
    
    channel_->stop();
    // 잠시 대기하여 비동기 작업 완료
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    channel_.reset();
    started_ = false;
}

void Serial::send(const std::string& data) {
    if (is_connected() && channel_) {
        channel_->async_write_copy(
            reinterpret_cast<const uint8_t*>(data.c_str()), 
            data.size()
        );
    }
}

void Serial::send_line(const std::string& line) {
    send(line + "\n");
}

bool Serial::is_connected() const {
    return channel_ && channel_->is_connected();
}

IChannel& Serial::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    if (channel_) {
        setup_internal_handlers();
    }
    return *this;
}

IChannel& Serial::on_connect(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
    return *this;
}

IChannel& Serial::on_disconnect(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
    return *this;
}

IChannel& Serial::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    return *this;
}

IChannel& Serial::auto_start(bool start) {
    auto_start_ = start;
    if (start && !started_) {
        this->start();
    }
    return *this;
}

IChannel& Serial::auto_manage(bool manage) {
    auto_manage_ = manage;
    if (manage && !started_) {
        auto_start(true);
    }
    return *this;
}

void Serial::set_baud_rate(uint32_t baud_rate) {
    baud_rate_ = baud_rate;
}

void Serial::set_data_bits(int data_bits) {
    data_bits_ = data_bits;
}

void Serial::set_stop_bits(int stop_bits) {
    stop_bits_ = stop_bits;
}

void Serial::set_parity(const std::string& parity) {
    parity_ = parity;
}

void Serial::set_flow_control(const std::string& flow_control) {
    flow_control_ = flow_control;
}

void Serial::setup_internal_handlers() {
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

void Serial::notify_state_change(common::LinkState state) {
    switch (state) {
        case common::LinkState::Connecting:
            // Connecting state - no action needed, just log
            break;
        case common::LinkState::Connected:
            if (connect_handler_) connect_handler_();
            break;
        case common::LinkState::Closed:
            if (disconnect_handler_) disconnect_handler_();
            break;
        case common::LinkState::Error:
            if (error_handler_) error_handler_("Serial connection error occurred");
            break;
        default:
            break;
    }
}

} // namespace wrapper
} // namespace unilink
