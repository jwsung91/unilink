#pragma once

#include <memory>
#include <functional>
#include <string>

namespace unilink {
namespace builder {

// 모든 Builder의 공통 인터페이스
template<typename T>
class IBuilder {
public:
    using DataHandler = std::function<void(const std::string&)>;
    using ConnectHandler = std::function<void()>;
    using DisconnectHandler = std::function<void()>;
    using ErrorHandler = std::function<void(const std::string&)>;

    virtual ~IBuilder() = default;
    
    // 공통 설정 메서드들
    virtual IBuilder& on_data(DataHandler handler) = 0;
    virtual IBuilder& on_connect(ConnectHandler handler) = 0;
    virtual IBuilder& on_disconnect(DisconnectHandler handler) = 0;
    virtual IBuilder& on_error(ErrorHandler handler) = 0;
    virtual IBuilder& auto_start(bool start = true) = 0;
    virtual IBuilder& auto_manage(bool manage = true) = 0;
    
    // 빌드 메서드
    virtual std::unique_ptr<T> build() = 0;
};

} // namespace builder
} // namespace unilink
