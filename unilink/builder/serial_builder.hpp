#pragma once

#include <memory>
#include <functional>
#include <string>
#include <chrono>

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/ichannel.hpp"

namespace unilink {
namespace builder {

class SerialBuilder : public IBuilder<wrapper::Serial> {
public:
    SerialBuilder(const std::string& device, uint32_t baud_rate);
    
    // IBuilder 구현
    IBuilder& on_data(DataHandler handler) override;
    IBuilder& on_connect(ConnectHandler handler) override;
    IBuilder& on_disconnect(DisconnectHandler handler) override;
    IBuilder& on_error(ErrorHandler handler) override;
    IBuilder& auto_start(bool start = true) override;
    IBuilder& auto_manage(bool manage = true) override;
    std::unique_ptr<wrapper::Serial> build() override;
    
    // 시리얼 전용 설정 메서드들
    SerialBuilder& device(const std::string& device);
    SerialBuilder& baud_rate(uint32_t baud_rate);
    SerialBuilder& data_bits(int data_bits);
    SerialBuilder& stop_bits(int stop_bits);
    SerialBuilder& parity(const std::string& parity);
    SerialBuilder& flow_control(const std::string& flow_control);
    SerialBuilder& timeout(std::chrono::milliseconds timeout);

private:
    std::string device_;
    uint32_t baud_rate_;
    int data_bits_ = 8;
    int stop_bits_ = 1;
    std::string parity_ = "none";
    std::string flow_control_ = "none";
    std::chrono::milliseconds timeout_{1000};
    
    // 이벤트 핸들러들
    DataHandler data_handler_;
    ConnectHandler connect_handler_;
    DisconnectHandler disconnect_handler_;
    ErrorHandler error_handler_;
    
    // 설정
    bool auto_start_ = false;
    bool auto_manage_ = false;
};

} // namespace builder
} // namespace unilink
