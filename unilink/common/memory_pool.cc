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
    
    // Track usage for auto-tuning
    {
        std::lock_guard<std::mutex> usage_lock(usage_mutex_);
        size_usage_count_[size]++;
    }
    
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
        
        // Update statistics with lock
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.pool_hits++;
            stats_.total_allocations++;
            stats_.total_allocation_time += duration.count();
            stats_.hits_by_size[size]++;
        }
        
        return std::move(it->data);
    } else {
        // Pool miss - create new buffer
        bucket.misses++;
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Update statistics with lock
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.pool_misses++;
            stats_.total_allocations++;
            stats_.total_allocation_time += duration.count();
            stats_.misses_by_size[size]++;
        }
        
        auto buffer = create_buffer(bucket.size);
        
        // Track memory allocation
        if (buffer) {
            total_memory_allocated_ += bucket.size;  // Use actual allocated size, not requested size
            size_t current_peak = peak_memory_usage_.load();
            while (total_memory_allocated_ > current_peak && 
                   !peak_memory_usage_.compare_exchange_weak(current_peak, total_memory_allocated_)) {
                // Retry until we successfully update the peak
            }
        }
        
        return buffer;
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
        
        // Note: Memory tracking is not updated here because the buffer is being
        // returned to the pool, not actually deallocated. The memory remains allocated.
    }
    // If pool is full, let the buffer be destroyed automatically
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Update deallocation time with lock
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_deallocation_time += duration.count();
    }
}

MemoryPool::PoolStats MemoryPool::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    PoolStats result = stats_;
    result.current_pool_size = current_total_buffers_.load();
    result.current_memory_usage = total_memory_allocated_.load();
    result.peak_memory_usage = peak_memory_usage_.load();
    
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
            if (buffer.in_use) {
                used_memory += buffer.size;
            }
        }
    }
    
    total_memory = total_memory_allocated_.load();
    
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

// ============================================================================
// Performance Optimization Methods
// ============================================================================

void MemoryPool::auto_tune() {
    if (!auto_tune_enabled_.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> usage_lock(usage_mutex_);
    
    // Analyze usage patterns and adjust pool configuration
    for (const auto& [size, count] : size_usage_count_) {
        if (count > 100) { // Only tune for frequently used sizes
            double hit_rate = get_hit_rate_for_size(size);
            
            if (hit_rate < 0.5) { // Low hit rate, increase pool size
                optimize_for_size(size, 0.8);
            }
        }
    }
}

MemoryPool::PoolStats MemoryPool::get_detailed_stats() const {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    PoolStats detailed_stats = stats_;
    
    // Calculate averages
    if (detailed_stats.total_allocations > 0) {
        detailed_stats.average_allocation_time_ms = 
            static_cast<double>(detailed_stats.total_allocation_time) / 
            detailed_stats.total_allocations;
    }
    
    if (detailed_stats.pool_hits + detailed_stats.pool_misses > 0) {
        detailed_stats.average_deallocation_time_ms = 
            static_cast<double>(detailed_stats.total_deallocation_time) / 
            (detailed_stats.pool_hits + detailed_stats.pool_misses);
    }
    
    // Update memory usage
    detailed_stats.current_memory_usage = total_memory_allocated_.load();
    detailed_stats.peak_memory_usage = peak_memory_usage_.load();
    
    return detailed_stats;
}

void MemoryPool::optimize_for_size(size_t size, double target_hit_rate) {
    PoolBucket& bucket = get_bucket(size);
    std::lock_guard<std::mutex> lock(bucket.mutex);
    
    // Calculate current hit rate for this size
    double current_hit_rate = get_hit_rate_for_size(size);
    
    if (current_hit_rate < target_hit_rate) {
        // Increase pool size for this bucket
        size_t current_size = bucket.buffers.size();
        size_t target_size = static_cast<size_t>(current_size * 1.5); // 50% increase
        
        // Add more buffers to the pool
        for (size_t i = current_size; i < target_size && i < max_pool_size_ / BUCKET_SIZES.size(); ++i) {
            BufferInfo info;
            info.data = std::make_unique<uint8_t[]>(size);
            info.size = size;
            info.last_used = std::chrono::steady_clock::now();
            info.in_use = false;
            bucket.buffers.push_back(std::move(info));
        }
    }
}

double MemoryPool::get_hit_rate_for_size(size_t size) const {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    
    auto hits_it = stats_.hits_by_size.find(size);
    auto misses_it = stats_.misses_by_size.find(size);
    
    size_t hits = (hits_it != stats_.hits_by_size.end()) ? hits_it->second : 0;
    size_t misses = (misses_it != stats_.misses_by_size.end()) ? misses_it->second : 0;
    
    if (hits + misses == 0) {
        return 0.0;
    }
    
    return static_cast<double>(hits) / (hits + misses);
}

} // namespace common
} // namespace unilink
