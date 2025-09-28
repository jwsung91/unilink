#include "unilink/common/io_context_manager.hpp"
#include <iostream>
#include <future>
#include <chrono>

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
        try {
            io_thread_.join();
        } catch (...) {
            // join 실패 시 무시
        }
    }
    
    // 항상 새로운 io_context 생성하여 깨끗한 상태 보장
    ioc_ = std::make_unique<IoContext>();
    
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
    try {
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
        
        // 스레드 종료 대기 (안전한 방식)
        if (io_thread_.joinable()) {
            try {
                // 짧은 타임아웃으로 join 시도
                auto future = std::async(std::launch::async, [this]() {
                    io_thread_.join();
                });
                
                if (future.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout) {
                    // 타임아웃 시 detach
                    io_thread_.detach();
                }
            } catch (...) {
                // 예외 발생 시 detach
                try {
                    io_thread_.detach();
                } catch (...) {
                    // detach도 실패하면 무시
                }
            }
        }
        
        running_ = false;
        
        // 완전히 새로운 io_context 생성하여 상태 초기화
        ioc_.reset();
        work_guard_.reset();
    } catch (...) {
        // stop()에서 예외가 발생해도 무시
        running_ = false;
        ioc_.reset();
        work_guard_.reset();
    }
}

bool IoContextManager::is_running() const {
    return running_.load();
}

IoContextManager::~IoContextManager() {
    try {
        stop();
    } catch (...) {
        // 소멸자에서는 예외를 무시
        // 로깅은 하지 않음 (소멸자에서 로깅은 위험할 수 있음)
    }
}

} // namespace common
} // namespace unilink
