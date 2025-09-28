#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <boost/asio.hpp>

// Forward declarations
namespace unilink {
namespace common {
class IoContextManager;
}
namespace wrapper {
class TcpServer;
class TcpClient;
}
}

namespace unilink {
namespace builder {

/**
 * @brief 리소스 분리를 위한 개선된 아키텍처 제안
 * 
 * 현재 문제점:
 * 1. 서버와 클라이언트가 같은 io_context를 공유
 * 2. 하나의 컴포넌트가 블록되면 전체 시스템 영향
 * 3. 독립적인 생명주기 관리 불가
 * 
 * 개선 방안:
 * 1. 각 컴포넌트가 독립적인 io_context 사용
 * 2. 명시적인 리소스 관리
 * 3. 컴포넌트 간 격리
 */
class ResourceManager {
public:
    using IoContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;
    
    /**
     * @brief 독립적인 io_context 생성
     * 
     * 각 컴포넌트가 독립적인 io_context를 사용하여
     * 서로 영향을 주지 않도록 합니다.
     */
    static std::unique_ptr<IoContext> create_independent_context() {
        return std::make_unique<IoContext>();
    }
    
    /**
     * @brief 공유 io_context 생성 (현재 방식)
     * 
     * 메모리 효율성을 위해 공유 io_context를 사용합니다.
     * 하지만 컴포넌트 간 격리가 어렵습니다.
     */
    static IoContext& get_shared_context();
    
    /**
     * @brief 리소스 정책 선택
     */
    enum class ResourcePolicy {
        INDEPENDENT,  // 각 컴포넌트가 독립적인 io_context 사용
        SHARED        // 모든 컴포넌트가 공유 io_context 사용
    };
    
    /**
     * @brief 현재 리소스 정책
     */
    static ResourcePolicy get_current_policy() {
        return current_policy_;
    }
    
    /**
     * @brief 리소스 정책 설정
     */
    static void set_policy(ResourcePolicy policy) {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        current_policy_ = policy;
    }

private:
    static ResourcePolicy current_policy_;
    static std::mutex policy_mutex_;
};

/**
 * @brief 리소스 분리 테스트를 위한 헬퍼 클래스
 */
class ResourceIsolationTest {
public:
    /**
     * @brief 독립적인 리소스로 서버 생성
     */
    static std::unique_ptr<wrapper::TcpServer> create_isolated_server(uint16_t port);
    
    /**
     * @brief 독립적인 리소스로 클라이언트 생성
     */
    static std::unique_ptr<wrapper::TcpClient> create_isolated_client(const std::string& host, uint16_t port);
};

} // namespace builder
} // namespace unilink
