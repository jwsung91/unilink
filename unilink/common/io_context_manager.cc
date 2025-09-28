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
    
    if (!ioc_) {
        ioc_ = std::make_unique<IoContext>();
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
    
    // 스레드 종료 대기
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    
    running_ = false;
}

bool IoContextManager::is_running() const {
    return running_.load();
}

IoContextManager::~IoContextManager() {
    stop();
}

} // namespace common
} // namespace unilink
