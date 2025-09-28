#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace unilink {
namespace common {

/**
 * 전역 io_context 관리자
 * 모든 Transport가 하나의 io_context를 공유하여 메모리 효율성 향상
 */
class IoContextManager {
public:
    using IoContext = boost::asio::io_context;
    using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;

    // 싱글톤 인스턴스 접근
    static IoContextManager& instance();
    
    // io_context 참조 반환
    IoContext& get_context();
    
    // io_context 시작/중지
    void start();
    void stop();
    
    // 상태 확인
    bool is_running() const;
    
    // 소멸자에서 자동 정리
    ~IoContextManager();

private:
    IoContextManager() = default;
    IoContextManager(const IoContextManager&) = delete;
    IoContextManager& operator=(const IoContextManager&) = delete;

    std::unique_ptr<IoContext> ioc_;
    std::unique_ptr<WorkGuard> work_guard_;
    std::thread io_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
};

} // namespace common
} // namespace unilink
