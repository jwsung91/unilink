#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/auto_initializer.hpp"
#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

TcpClientBuilder::TcpClientBuilder(const std::string& host, uint16_t port)
    : host_(host), port_(port), auto_start_(false), auto_manage_(false), use_independent_context_(false) {}

std::unique_ptr<wrapper::TcpClient> TcpClientBuilder::build() {
    // IoContext 관리
    if (use_independent_context_) {
        // 독립적인 IoContext 사용 (테스트 격리용)
        // IoContextManager를 통해 독립적인 컨텍스트 생성
        auto independent_context = common::IoContextManager::instance().create_independent_context();
        // 현재는 기본 구현 유지, 향후 wrapper가 독립적인 컨텍스트를 받을 수 있도록 확장 가능
    } else {
        // 자동으로 IoContextManager 초기화 (기본 동작)
        AutoInitializer::ensure_io_context_running();
    }
    
    auto client = std::make_unique<wrapper::TcpClient>(host_, port_);
    
    // Apply configuration
    if (auto_start_) {
        client->auto_start(true);
    }
    
    if (auto_manage_) {
        client->auto_manage(true);
    }
    
    // Set callbacks
    if (on_data_) {
        client->on_data(on_data_);
    }
    
    if (on_connect_) {
        client->on_connect(on_connect_);
    }
    
    if (on_disconnect_) {
        client->on_disconnect(on_disconnect_);
    }
    
    if (on_error_) {
        client->on_error(on_error_);
    }
    
    return client;
}

TcpClientBuilder& TcpClientBuilder::auto_start(bool auto_start) {
    auto_start_ = auto_start;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::auto_manage(bool auto_manage) {
    auto_manage_ = auto_manage;
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_data(std::function<void(const std::string&)> handler) {
    on_data_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_connect(std::function<void()> handler) {
    on_connect_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_disconnect(std::function<void()> handler) {
    on_disconnect_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::on_error(std::function<void(const std::string&)> handler) {
    on_error_ = std::move(handler);
    return *this;
}

TcpClientBuilder& TcpClientBuilder::use_independent_context(bool use_independent) {
    use_independent_context_ = use_independent;
    return *this;
}

} // namespace builder
} // namespace unilink

