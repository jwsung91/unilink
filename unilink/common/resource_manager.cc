#include "unilink/common/resource_manager.hpp"
#include <iostream>

namespace unilink {
namespace common {

ResourceManager::~ResourceManager() {
    cleanup_all();
}

void ResourceManager::add_cleanup(ResourceManager::CleanupTask task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!cleaned_up_ && task) {
        cleanup_tasks_.push_back(std::move(task));
    }
}

void ResourceManager::cleanup_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (cleaned_up_) {
        return;
    }
    
    // 역순으로 정리 작업 실행 (LIFO)
    for (auto it = cleanup_tasks_.rbegin(); it != cleanup_tasks_.rend(); ++it) {
        try {
            if (*it) {
                (*it)();
            }
        } catch (const std::exception& e) {
            std::cerr << "ResourceManager cleanup error: " << e.what() << std::endl;
        }
    }
    
    cleanup_tasks_.clear();
    cleaned_up_ = true;
}

size_t ResourceManager::cleanup_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cleanup_tasks_.size();
}

} // namespace common
} // namespace unilink
