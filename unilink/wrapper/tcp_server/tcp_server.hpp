#pragma once

#include <memory>
#include <string>
#include <functional>

#include "unilink/wrapper/ichannel.hpp"
#include "unilink/interface/channel.hpp"

namespace unilink {
namespace wrapper {

class TcpServer : public IChannel {
public:
    explicit TcpServer(uint16_t port);
    explicit TcpServer(std::shared_ptr<interface::Channel> channel);
    ~TcpServer() override;

    // IChannel 구현
    void start() override;
    void stop() override;
    void send(const std::string& data) override;
    void send_line(const std::string& line) override;
    bool is_connected() const override;
    
    IChannel& on_data(DataHandler handler) override;
    IChannel& on_connect(ConnectHandler handler) override;
    IChannel& on_disconnect(DisconnectHandler handler) override;
    IChannel& on_error(ErrorHandler handler) override;
    
    IChannel& auto_start(bool start = true) override;
    IChannel& auto_manage(bool manage = true) override;

    // TCP 서버 전용 메서드들
    void set_max_connections(size_t max_connections);
    void set_timeout(std::chrono::milliseconds timeout);

private:
    void setup_internal_handlers();
    void notify_state_change(common::LinkState state);

private:
    uint16_t port_;
    std::shared_ptr<interface::Channel> channel_;
    
    // 이벤트 핸들러들
    DataHandler data_handler_;
    ConnectHandler connect_handler_;
    DisconnectHandler disconnect_handler_;
    ErrorHandler error_handler_;
    
    // 설정
    bool auto_start_ = false;
    bool auto_manage_ = false;
    bool started_ = false;
    
    // TCP 서버 전용 설정
    size_t max_connections_ = 1;
    std::chrono::milliseconds timeout_{5000};
};

} // namespace wrapper
} // namespace unilink
