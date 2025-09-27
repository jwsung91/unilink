#pragma once

#include <memory>
#include <functional>
#include <string>
#include <chrono>

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/ichannel.hpp"

namespace unilink {
namespace builder {

class TcpServerBuilder : public IBuilder<wrapper::TcpServer> {
public:
    explicit TcpServerBuilder(uint16_t port);
    
    // IBuilder 구현
    IBuilder& on_data(DataHandler handler) override;
    IBuilder& on_connect(ConnectHandler handler) override;
    IBuilder& on_disconnect(DisconnectHandler handler) override;
    IBuilder& on_error(ErrorHandler handler) override;
    IBuilder& auto_start(bool start = true) override;
    IBuilder& auto_manage(bool manage = true) override;
    std::unique_ptr<wrapper::TcpServer> build() override;
    
    // TCP 서버 전용 설정 메서드들
    TcpServerBuilder& port(uint16_t port);
    TcpServerBuilder& max_connections(size_t max_connections);
    TcpServerBuilder& timeout(std::chrono::milliseconds timeout);
    TcpServerBuilder& buffer_size(size_t buffer_size);

private:
    uint16_t port_;
    size_t max_connections_ = 1;
    std::chrono::milliseconds timeout_{5000};
    size_t buffer_size_ = 4096;
    
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
