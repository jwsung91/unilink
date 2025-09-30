#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <cstddef>

namespace unilink {
namespace common {

/**
 * @brief Simple memory pool for reducing allocation overhead
 * 
 * This pool pre-allocates a fixed number of buffers and reuses them,
 * reducing the overhead of frequent memory allocations in high-throughput scenarios.
 */
template<typename T, size_t PoolSize = 100>
class MemoryPool {
public:
    using BufferType = std::vector<T>;
    using BufferPtr = std::unique_ptr<BufferType>;
    
    MemoryPool() {
        // Pre-allocate buffers
        for (size_t i = 0; i < PoolSize; ++i) {
            available_buffers_.push_back(std::make_unique<BufferType>());
        }
    }
    
    ~MemoryPool() = default;
    
    // Non-copyable, non-movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;
    
    /**
     * @brief Get a buffer from the pool
     * @return BufferPtr A unique_ptr to a buffer, or nullptr if pool is empty
     */
    BufferPtr acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (available_buffers_.empty()) {
            return nullptr; // Pool exhausted, caller should fall back to normal allocation
        }
        
        auto buffer = std::move(available_buffers_.back());
        available_buffers_.pop_back();
        return buffer;
    }
    
    /**
     * @brief Return a buffer to the pool
     * @param buffer The buffer to return (will be cleared)
     */
    void release(BufferPtr buffer) {
        if (!buffer) return;
        
        // Clear the buffer before returning to pool
        buffer->clear();
        buffer->shrink_to_fit();
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (available_buffers_.size() < PoolSize) {
            available_buffers_.push_back(std::move(buffer));
        }
        // If pool is full, let the buffer be destroyed
    }
    
    /**
     * @brief Get current pool statistics
     * @return std::pair<size_t, size_t> Available buffers, Total pool size
     */
    std::pair<size_t, size_t> get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return {available_buffers_.size(), PoolSize};
    }

private:
    mutable std::mutex mutex_;
    std::vector<BufferPtr> available_buffers_;
};

/**
 * @brief Global memory pool instance for byte buffers
 */
class GlobalMemoryPool {
public:
    static MemoryPool<uint8_t>& instance() {
        static MemoryPool<uint8_t> pool;
        return pool;
    }
    
    // Non-copyable, non-movable
    GlobalMemoryPool() = delete;
    GlobalMemoryPool(const GlobalMemoryPool&) = delete;
    GlobalMemoryPool& operator=(const GlobalMemoryPool&) = delete;
};

} // namespace common
} // namespace unilink
