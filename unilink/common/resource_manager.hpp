#pragma once

#include <functional>
#include <vector>
#include <mutex>

namespace unilink {
namespace common {

/**
 * RAII 기반 리소스 관리자
 * 등록된 정리 작업들을 자동으로 실행하여 메모리 누수 방지
 */
class ResourceManager {
public:
    using CleanupTask = std::function<void()>;
    
    ResourceManager() = default;
    ~ResourceManager();
    
    // 복사/이동 방지
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;
    
    // 정리 작업 등록
    void add_cleanup(CleanupTask task);
    
    // 모든 정리 작업 실행
    void cleanup_all();
    
    // 정리 작업 개수 반환
    size_t cleanup_count() const;

private:
    mutable std::mutex mutex_;
    std::vector<CleanupTask> cleanup_tasks_;
    bool cleaned_up_{false};
};

} // namespace common
} // namespace unilink
