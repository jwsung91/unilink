#include "unilink/builder/serial_builder.hpp"

#include "unilink/wrapper/serial/serial.hpp"

namespace unilink {
namespace builder {

SerialBuilder::SerialBuilder(const std::string& device, uint32_t baud_rate) 
    : device_(device), baud_rate_(baud_rate) {}

IBuilder<wrapper::Serial>& SerialBuilder::on_data(DataHandler handler) {
    data_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::Serial>& SerialBuilder::on_connect(ConnectHandler handler) {
    connect_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::Serial>& SerialBuilder::on_disconnect(DisconnectHandler handler) {
    disconnect_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::Serial>& SerialBuilder::on_error(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    return *this;
}

IBuilder<wrapper::Serial>& SerialBuilder::auto_start(bool start) {
    auto_start_ = start;
    return *this;
}

IBuilder<wrapper::Serial>& SerialBuilder::auto_manage(bool manage) {
    auto_manage_ = manage;
    return *this;
}

std::unique_ptr<wrapper::Serial> SerialBuilder::build() {
    auto serial = std::make_unique<wrapper::Serial>(device_, baud_rate_);
    
    // 설정된 핸들러들 적용
    if (data_handler_) serial->on_data(data_handler_);
    if (connect_handler_) serial->on_connect(connect_handler_);
    if (disconnect_handler_) serial->on_disconnect(disconnect_handler_);
    if (error_handler_) serial->on_error(error_handler_);
    
    // 설정 적용
    if (auto_start_) serial->auto_start();
    if (auto_manage_) serial->auto_manage();
    
    // 시리얼 전용 설정 적용
    serial->set_data_bits(data_bits_);
    serial->set_stop_bits(stop_bits_);
    serial->set_parity(parity_);
    serial->set_flow_control(flow_control_);
    
    return serial;
}

SerialBuilder& SerialBuilder::device(const std::string& device) {
    device_ = device;
    return *this;
}

SerialBuilder& SerialBuilder::baud_rate(uint32_t baud_rate) {
    baud_rate_ = baud_rate;
    return *this;
}

SerialBuilder& SerialBuilder::data_bits(int data_bits) {
    data_bits_ = data_bits;
    return *this;
}

SerialBuilder& SerialBuilder::stop_bits(int stop_bits) {
    stop_bits_ = stop_bits;
    return *this;
}

SerialBuilder& SerialBuilder::parity(const std::string& parity) {
    parity_ = parity;
    return *this;
}

SerialBuilder& SerialBuilder::flow_control(const std::string& flow_control) {
    flow_control_ = flow_control;
    return *this;
}

SerialBuilder& SerialBuilder::timeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
    return *this;
}

} // namespace builder
} // namespace unilink
