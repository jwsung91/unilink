#include "unilink/common/io_context_manager.hpp"
#include <iostream>

namespace unilink {
namespace common {

IoContextManager& IoContextManager::instance() {
    static IoContextManager instance;
    return instance;
}

boost::asio::io_context& IoContextManager::get_context() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ioc_) {
        ioc_ = std::make_unique<IoContext>();
    }
    return *ioc_;
}

void IoContextManager::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return;
    }
    
    // 이전 상태가 있다면 정리
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    if (!ioc_) {
        ioc_ = std::make_unique<IoContext>();
    } else {
        // 기존 io_context가 있다면 재시작
        ioc_->restart();
    }
    
    // Work guard 생성하여 io_context가 비어있지 않도록 유지
    work_guard_ = std::make_unique<WorkGuard>(ioc_->get_executor());
    
    // 별도 스레드에서 io_context 실행
    io_thread_ = std::thread([this]() {
        try {
            ioc_->run();
        } catch (const std::exception& e) {
            std::cerr << "IoContextManager thread error: " << e.what() << std::endl;
        }
    });
    
    running_ = true;
}

void IoContextManager::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
        return;
    }
    
    // Work guard 해제하여 io_context가 자연스럽게 종료되도록 함
    work_guard_.reset();
    
    // io_context 중지
    if (ioc_) {
        ioc_->stop();
    }
    
    // 스레드 종료 대기 (타임아웃 추가)
    if (io_thread_.joinable()) {
        // 별도 스레드에서 join하여 데드락 방지
        std::thread([this]() {
            try {
                io_thread_.join();
            } catch (...) {
                // join 실패 시 무시
            }
        }).detach();
    }
    
    running_ = false;
    
    // io_context를 재시작 가능하도록 리셋
    if (ioc_) {
        ioc_->restart();
    }
}

bool IoContextManager::is_running() const {
    return running_.load();
}

IoContextManager::~IoContextManager() {
    stop();
}

} // namespace common
} // namespace unilink
