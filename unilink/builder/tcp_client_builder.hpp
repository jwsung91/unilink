#pragma once

#include <memory>
#include <functional>
#include <string>
#include <chrono>

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/ichannel.hpp"

namespace unilink {
namespace builder {

class TcpClientBuilder : public IBuilder<wrapper::TcpClient> {
public:
    TcpClientBuilder(const std::string& host, uint16_t port);
    
    // IBuilder 구현
    IBuilder& on_data(DataHandler handler) override;
    IBuilder& on_connect(ConnectHandler handler) override;
    IBuilder& on_disconnect(DisconnectHandler handler) override;
    IBuilder& on_error(ErrorHandler handler) override;
    IBuilder& auto_start(bool start = true) override;
    IBuilder& auto_manage(bool manage = true) override;
    std::unique_ptr<wrapper::TcpClient> build() override;
    
    // TCP 클라이언트 전용 설정 메서드들
    TcpClientBuilder& host(const std::string& host);
    TcpClientBuilder& port(uint16_t port);
    TcpClientBuilder& retry_interval(std::chrono::milliseconds interval);
    TcpClientBuilder& max_retries(int max_retries);
    TcpClientBuilder& connection_timeout(std::chrono::milliseconds timeout);
    TcpClientBuilder& keep_alive(bool enable = true);

private:
    std::string host_;
    uint16_t port_;
    std::chrono::milliseconds retry_interval_{2000};
    int max_retries_ = -1; // -1 means unlimited
    std::chrono::milliseconds connection_timeout_{5000};
    bool keep_alive_ = true;
    
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
