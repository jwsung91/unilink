#include "unilink/builder/resource_manager.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"

namespace unilink {
namespace builder {

ResourceManager::ResourcePolicy ResourceManager::current_policy_ = ResourceManager::ResourcePolicy::SHARED;
std::mutex ResourceManager::policy_mutex_;

ResourceManager::IoContext& ResourceManager::get_shared_context() {
    return common::IoContextManager::instance().get_context();
}

std::unique_ptr<wrapper::TcpServer> ResourceIsolationTest::create_isolated_server(uint16_t port) {
    // 독립적인 io_context 사용
    auto ioc = ResourceManager::create_independent_context();
    
    // TcpServer는 현재 공유 io_context만 지원하므로
    // 이는 개념적 구현입니다.
    // 실제로는 TcpServer 생성자를 수정해야 합니다.
    
    return nullptr; // 구현 필요
}

std::unique_ptr<wrapper::TcpClient> ResourceIsolationTest::create_isolated_client(const std::string& host, uint16_t port) {
    // TcpClient는 이미 독립적인 io_context를 사용합니다.
    return std::make_unique<wrapper::TcpClient>(host, port);
}

} // namespace builder
} // namespace unilink
