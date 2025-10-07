#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstddef>
#include <chrono>
#include <unordered_map>
#include <array>

namespace unilink {
namespace common {

/**
 * @brief Advanced memory pool with dynamic sizing and performance monitoring
 * 
 * Features:
 * - Dynamic pool sizing based on usage patterns
 * - Multiple buffer sizes for different use cases
 * - Performance monitoring and statistics
 * - Thread-safe operations with minimal locking
 * - Automatic cleanup and memory management
 */
class MemoryPool {
public:
    struct PoolStats {
        // Core statistics - maintained centrally to avoid double counting
        size_t total_allocations{0};
        size_t pool_hits{0};
        size_t pool_misses{0};
        size_t current_pool_size{0};
        size_t max_pool_size{0};
        
        // Performance metrics
        std::chrono::milliseconds::rep total_allocation_time{0};
        std::chrono::milliseconds::rep total_deallocation_time{0};
        double average_allocation_time_ms{0.0};
        double average_deallocation_time_ms{0.0};
        size_t peak_memory_usage{0};
        size_t current_memory_usage{0};
        
        // Hit rate by buffer size - thread-safe access
        std::unordered_map<size_t, size_t> hits_by_size;
        std::unordered_map<size_t, size_t> misses_by_size;
    };

    struct BufferInfo {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        std::chrono::steady_clock::time_point last_used;
        bool in_use{false};
    };

    // Predefined buffer sizes for common use cases
    enum class BufferSize : size_t {
        SMALL = 1024,      // 1KB - small messages
        MEDIUM = 4096,     // 4KB - typical network packets
        LARGE = 16384,     // 16KB - large data transfers
        XLARGE = 65536     // 64KB - bulk operations
    };

    explicit MemoryPool(size_t initial_pool_size = 50, size_t max_pool_size = 200);
    ~MemoryPool() = default;

    // Non-copyable, non-movable
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;

    /**
     * @brief Acquire a buffer of specified size
     * @param size Required buffer size
     * @return std::unique_ptr<uint8_t[]> Buffer or nullptr if allocation fails
     */
    std::unique_ptr<uint8_t[]> acquire(size_t size);

    /**
     * @brief Acquire a buffer of predefined size
     * @param buffer_size Predefined buffer size
     * @return std::unique_ptr<uint8_t[]> Buffer or nullptr if allocation fails
     */
    std::unique_ptr<uint8_t[]> acquire(BufferSize buffer_size);

    /**
     * @brief Release a buffer back to the pool
     * @param buffer Buffer to release
     * @param size Size of the buffer
     */
    void release(std::unique_ptr<uint8_t[]> buffer, size_t size);

    /**
     * @brief Get comprehensive pool statistics
     * @return PoolStats Current pool statistics
     */
    PoolStats get_stats() const;

    /**
     * @brief Get hit rate (pool hits / total allocations)
     * @return double Hit rate as percentage (0.0 to 1.0)
     */
    double get_hit_rate() const;

    /**
     * @brief Resize the pool dynamically
     * @param new_size New pool size
     */
    void resize_pool(size_t new_size);

    /**
     * @brief Clean up unused buffers older than specified duration
     * @param max_age Maximum age of buffers to keep
     */
    void cleanup_old_buffers(std::chrono::milliseconds max_age = std::chrono::minutes(5));

    /**
     * @brief Get memory usage information
     * @return std::pair<size_t, size_t> Used memory, Total allocated memory
     */
    std::pair<size_t, size_t> get_memory_usage() const;

    /**
     * @brief Auto-tune pool based on usage patterns
     * Analyzes hit rates and adjusts pool configuration
     */
    void auto_tune();

    /**
     * @brief Get detailed performance metrics
     * @return PoolStats with enhanced metrics
     */
    PoolStats get_detailed_stats() const;

    /**
     * @brief Optimize pool for specific buffer size
     * @param size Buffer size to optimize for
     * @param target_hit_rate Target hit rate (0.0 to 1.0)
     */
    void optimize_for_size(size_t size, double target_hit_rate = 0.8);

    /**
     * @brief Get hit rate for specific buffer size
     * @param size Buffer size
     * @return Hit rate for that size
     */
    double get_hit_rate_for_size(size_t size) const;

private:
    struct PoolBucket {
        std::vector<BufferInfo> buffers;
        mutable std::mutex mutex;
        size_t size;
        std::atomic<size_t> hits{0};
        std::atomic<size_t> misses{0};
        
        // Default constructor
        PoolBucket() = default;
        
        // Move constructor
        PoolBucket(PoolBucket&& other) noexcept
            : buffers(std::move(other.buffers))
            , size(other.size)
            , hits(other.hits.load())
            , misses(other.misses.load()) {
        }
        
        // Move assignment
        PoolBucket& operator=(PoolBucket&& other) noexcept {
            if (this != &other) {
                buffers = std::move(other.buffers);
                size = other.size;
                hits.store(other.hits.load());
                misses.store(other.misses.load());
            }
            return *this;
        }
        
        // Delete copy operations
        PoolBucket(const PoolBucket&) = delete;
        PoolBucket& operator=(const PoolBucket&) = delete;
    };

    // Find the appropriate bucket for the given size
    PoolBucket& get_bucket(size_t size);
    const PoolBucket& get_bucket(size_t size) const;

    // Create a new buffer
    std::unique_ptr<uint8_t[]> create_buffer(size_t size);

    // Clean up expired buffers in a bucket
    void cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age);

    mutable std::mutex stats_mutex_;
    PoolStats stats_;
    
    std::vector<PoolBucket> buckets_;
    size_t max_pool_size_;
    std::atomic<size_t> current_total_buffers_{0};
    
    // Performance optimization
    std::atomic<size_t> total_memory_allocated_{0};
    std::atomic<size_t> peak_memory_usage_{0};
    std::chrono::steady_clock::time_point last_auto_tune_;
    std::atomic<bool> auto_tune_enabled_{true};
    
    // Dynamic sizing
    std::unordered_map<size_t, size_t> size_usage_count_;
    std::unordered_map<size_t, double> size_hit_rates_;
    mutable std::mutex usage_mutex_;
    
    // Predefined bucket sizes
    static constexpr std::array<size_t, 4> BUCKET_SIZES = {
        static_cast<size_t>(BufferSize::SMALL),
        static_cast<size_t>(BufferSize::MEDIUM),
        static_cast<size_t>(BufferSize::LARGE),
        static_cast<size_t>(BufferSize::XLARGE)
    };
};

/**
 * @brief Global memory pool instance
 */
class GlobalMemoryPool {
public:
    static MemoryPool& instance() {
        static MemoryPool pool;
        return pool;
    }
    
    // Non-copyable, non-movable
    GlobalMemoryPool() = delete;
    GlobalMemoryPool(const GlobalMemoryPool&) = delete;
    GlobalMemoryPool& operator=(const GlobalMemoryPool&) = delete;
};

/**
 * @brief RAII wrapper for memory pool buffers
 */
class PooledBuffer {
public:
    explicit PooledBuffer(size_t size);
    explicit PooledBuffer(MemoryPool::BufferSize buffer_size);
    ~PooledBuffer();

    // Non-copyable, movable
    PooledBuffer(const PooledBuffer&) = delete;
    PooledBuffer& operator=(const PooledBuffer&) = delete;
    PooledBuffer(PooledBuffer&& other) noexcept;
    PooledBuffer& operator=(PooledBuffer&& other) noexcept;

    uint8_t* data() const { return buffer_.get(); }
    size_t size() const { return size_; }
    bool valid() const { return buffer_ != nullptr; }

    // Implicit conversion to raw pointer for compatibility
    operator uint8_t*() const { return data(); }

private:
    std::unique_ptr<uint8_t[]> buffer_;
    size_t size_;
    MemoryPool* pool_;
};

} // namespace common
} // namespace unilink
