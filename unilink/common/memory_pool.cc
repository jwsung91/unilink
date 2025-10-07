#include "unilink/common/memory_pool.hpp"
#include <algorithm>
#include <iostream>
#include <array>

namespace unilink {
namespace common {

// ============================================================================
// EnhancedMemoryPool Implementation
// ============================================================================

MemoryPool::MemoryPool(size_t initial_pool_size, size_t max_pool_size)
    : max_pool_size_(max_pool_size) {
    // Initialize buckets for each predefined size
    buckets_.reserve(BUCKET_SIZES.size());
    for (size_t bucket_size : BUCKET_SIZES) {
        PoolBucket bucket;
        bucket.size = bucket_size;
        bucket.buffers.reserve(initial_pool_size / BUCKET_SIZES.size());
        buckets_.push_back(std::move(bucket));
    }
    
    stats_.max_pool_size = max_pool_size;
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire(size_t size) {
    auto start_time = std::chrono::steady_clock::now();
    
    PoolBucket& bucket = get_bucket(size);
    std::lock_guard<std::mutex> lock(bucket.mutex);
    
    // Try to find an available buffer in the pool
    auto it = std::find_if(bucket.buffers.begin(), bucket.buffers.end(),
                          [](const BufferInfo& info) { return !info.in_use; });
    
    if (it != bucket.buffers.end()) {
        // Found an available buffer
        it->in_use = true;
        it->last_used = std::chrono::steady_clock::now();
        bucket.hits++;
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.pool_hits++;
            stats_.total_allocation_time += duration;
        }
        
        return std::move(it->data);
    } else {
        // Pool miss - create new buffer
        bucket.misses++;
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.pool_misses++;
            stats_.total_allocation_time += duration;
        }
        
        return create_buffer(bucket.size);
    }
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire(BufferSize buffer_size) {
    return acquire(static_cast<size_t>(buffer_size));
}

void MemoryPool::release(std::unique_ptr<uint8_t[]> buffer, size_t size) {
    if (!buffer) return;
    
    auto start_time = std::chrono::steady_clock::now();
    
    PoolBucket& bucket = get_bucket(size);
    std::lock_guard<std::mutex> lock(bucket.mutex);
    
    // Check if we can add this buffer back to the pool
    if (bucket.buffers.size() < max_pool_size_ / BUCKET_SIZES.size()) {
        BufferInfo info;
        info.data = std::move(buffer);
        info.size = bucket.size;
        info.last_used = std::chrono::steady_clock::now();
        info.in_use = false;
        
        bucket.buffers.push_back(std::move(info));
        current_total_buffers_++;
    }
    // If pool is full, let the buffer be destroyed automatically
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_deallocation_time += duration;
    }
}

MemoryPool::PoolStats MemoryPool::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    PoolStats result = stats_;
    result.current_pool_size = current_total_buffers_.load();
    
    // Add bucket-specific statistics
    size_t total_hits = 0;
    size_t total_misses = 0;
    for (const auto& bucket : buckets_) {
        std::lock_guard<std::mutex> bucket_lock(bucket.mutex);
        total_hits += bucket.hits.load();
        total_misses += bucket.misses.load();
    }
    
    result.pool_hits = total_hits;
    result.pool_misses = total_misses;
    result.total_allocations = total_hits + total_misses;
    
    return result;
}

double MemoryPool::get_hit_rate() const {
    // Get current stats to calculate hit rate
    auto stats = get_stats();
    
    size_t total = stats.pool_hits + stats.pool_misses;
    if (total == 0) return 0.0;
    
    return static_cast<double>(stats.pool_hits) / static_cast<double>(total);
}

void MemoryPool::resize_pool(size_t new_size) {
    max_pool_size_ = new_size;
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.max_pool_size = new_size;
    }
}

void MemoryPool::cleanup_old_buffers(std::chrono::milliseconds max_age) {
    for (auto& bucket : buckets_) {
        cleanup_bucket(bucket, max_age);
    }
}

std::pair<size_t, size_t> MemoryPool::get_memory_usage() const {
    size_t used_memory = 0;
    size_t total_memory = 0;
    
    for (const auto& bucket : buckets_) {
        std::lock_guard<std::mutex> lock(bucket.mutex);
        
        for (const auto& buffer : bucket.buffers) {
            total_memory += buffer.size;
            if (buffer.in_use) {
                used_memory += buffer.size;
            }
        }
    }
    
    return {used_memory, total_memory};
}

MemoryPool::PoolBucket& MemoryPool::get_bucket(size_t size) {
    // Find the smallest bucket that can accommodate the requested size
    for (auto& bucket : buckets_) {
        if (bucket.size >= size) {
            return bucket;
        }
    }
    
    // If no bucket is large enough, return the largest one
    return buckets_.back();
}

const MemoryPool::PoolBucket& MemoryPool::get_bucket(size_t size) const {
    // Find the smallest bucket that can accommodate the requested size
    for (const auto& bucket : buckets_) {
        if (bucket.size >= size) {
            return bucket;
        }
    }
    
    // If no bucket is large enough, return the largest one
    return buckets_.back();
}

std::unique_ptr<uint8_t[]> MemoryPool::create_buffer(size_t size) {
    try {
        return std::make_unique<uint8_t[]>(size);
    } catch (const std::bad_alloc& e) {
        // If allocation fails, return nullptr
        // This will cause the caller to fall back to regular allocation
        return nullptr;
    }
}

void MemoryPool::cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age) {
    std::lock_guard<std::mutex> lock(bucket.mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto cutoff_time = now - max_age;
    
    bucket.buffers.erase(
        std::remove_if(bucket.buffers.begin(), bucket.buffers.end(),
                      [&cutoff_time](const BufferInfo& info) {
                          return !info.in_use && info.last_used < cutoff_time;
                      }),
        bucket.buffers.end()
    );
}

// ============================================================================
// PooledBuffer Implementation
// ============================================================================

PooledBuffer::PooledBuffer(size_t size)
    : size_(size), pool_(&GlobalMemoryPool::instance()) {
    buffer_ = pool_->acquire(size);
}

PooledBuffer::PooledBuffer(MemoryPool::BufferSize buffer_size)
    : size_(static_cast<size_t>(buffer_size)), pool_(&GlobalMemoryPool::instance()) {
    buffer_ = pool_->acquire(buffer_size);
}

PooledBuffer::~PooledBuffer() {
    if (buffer_ && pool_) {
        pool_->release(std::move(buffer_), size_);
    }
}

PooledBuffer::PooledBuffer(PooledBuffer&& other) noexcept
    : buffer_(std::move(other.buffer_)), size_(other.size_), pool_(other.pool_) {
    other.buffer_ = nullptr;
    other.size_ = 0;
    other.pool_ = nullptr;
}

PooledBuffer& PooledBuffer::operator=(PooledBuffer&& other) noexcept {
    if (this != &other) {
        // Release current buffer
        if (buffer_ && pool_) {
            pool_->release(std::move(buffer_), size_);
        }
        
        // Move from other
        buffer_ = std::move(other.buffer_);
        size_ = other.size_;
        pool_ = other.pool_;
        
        // Reset other
        other.buffer_ = nullptr;
        other.size_ = 0;
        other.pool_ = nullptr;
    }
    return *this;
}

} // namespace common
} // namespace unilink
