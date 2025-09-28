#pragma once

#include <memory>
#include <string>
#include <functional>

#include "unilink/wrapper/ichannel.hpp"
#include "unilink/factory/channel_factory.hpp"

namespace unilink {
namespace wrapper {

/**
 * 개선된 TCP Server Wrapper
 * - 공유 io_context 사용
 * - 메모리 누수 방지
 * - 자동 리소스 관리
 */
class TcpServer : public ChannelInterface {
public:
    explicit TcpServer(uint16_t port);
    explicit TcpServer(std::shared_ptr<interface::Channel> channel);
    ~TcpServer() = default;

    // IChannel 인터페이스 구현
    void start() override;
    void stop() override;
    void send(const std::string& data) override;
    bool is_connected() const override;

    ChannelInterface& on_data(DataHandler handler) override;
    ChannelInterface& on_connect(ConnectHandler handler) override;
    ChannelInterface& on_disconnect(DisconnectHandler handler) override;
    ChannelInterface& on_error(ErrorHandler handler) override;

    ChannelInterface& auto_start(bool start = true) override;
    ChannelInterface& auto_manage(bool manage = true) override;

    void send_line(const std::string& line) override;
    // void send_binary(const std::vector<uint8_t>& data) override;

private:
    void setup_internal_handlers();
    void handle_bytes(const uint8_t* data, size_t size);
    void handle_state(common::LinkState state);

    std::shared_ptr<interface::Channel> channel_;
    uint16_t port_;
    bool started_{false};
    bool auto_start_{false};
    bool auto_manage_{false};

    // 사용자 콜백들
    DataHandler on_data_;
    ConnectHandler on_connect_;
    DisconnectHandler on_disconnect_;
    ErrorHandler on_error_;
};

} // namespace wrapper
} // namespace unilink
