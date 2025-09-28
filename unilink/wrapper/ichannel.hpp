#pragma once

#include <functional>
#include <memory>
#include <string>

namespace unilink {
namespace wrapper {

// 모든 Wrapper 통신 클래스의 공통 인터페이스
class ChannelInterface {
public:
    using DataHandler = std::function<void(const std::string&)>;
    using ConnectHandler = std::function<void()>;
    using DisconnectHandler = std::function<void()>;
    using ErrorHandler = std::function<void(const std::string&)>;

    virtual ~ChannelInterface() = default;
    
    // 공통 메서드들
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send(const std::string& data) = 0;
    virtual void send_line(const std::string& line) = 0;
    virtual bool is_connected() const = 0;
    
    // 이벤트 핸들러 설정
    virtual ChannelInterface& on_data(DataHandler handler) = 0;
    virtual ChannelInterface& on_connect(ConnectHandler handler) = 0;
    virtual ChannelInterface& on_disconnect(DisconnectHandler handler) = 0;
    virtual ChannelInterface& on_error(ErrorHandler handler) = 0;
    
    // 편의 메서드들
    virtual ChannelInterface& auto_start(bool start = true) = 0;
    virtual ChannelInterface& auto_manage(bool manage = true) = 0;
};

} // namespace wrapper
} // namespace unilink
