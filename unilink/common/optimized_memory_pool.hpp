#pragma once

#include <memory>
#include <unordered_map>

#include "memory_pool.hpp"

namespace unilink::common {

/**
 * @brief Optimized memory pool with size-specific pools
 *
 * This class provides optimized memory allocation by using separate pools
 * for different buffer sizes, improving performance for specific use cases.
 */
class OptimizedMemoryPool {
 public:
  // Size categories for different buffer types
  enum class SizeCategory {
    SMALL,   // 1KB - 4KB
    MEDIUM,  // 8KB - 32KB
    LARGE    // 64KB+
  };

  explicit OptimizedMemoryPool(size_t small_initial = 600, size_t small_max = 2000,   // 1KB-4KB
                               size_t medium_initial = 200, size_t medium_max = 800,  // 8KB-32KB
                               size_t large_initial = 100, size_t large_max = 400     // 64KB+
  );

  ~OptimizedMemoryPool() = default;

  // Non-copyable, non-movable
  OptimizedMemoryPool(const OptimizedMemoryPool&) = delete;
  OptimizedMemoryPool& operator=(const OptimizedMemoryPool&) = delete;
  OptimizedMemoryPool(OptimizedMemoryPool&&) = delete;
  OptimizedMemoryPool& operator=(OptimizedMemoryPool&&) = delete;

  /**
   * @brief Acquire a buffer of the specified size
   * @param size Buffer size in bytes
   * @return Unique pointer to the buffer, or nullptr if allocation fails
   */
  std::unique_ptr<uint8_t[]> acquire(size_t size);

  /**
   * @brief Release a buffer back to the appropriate pool
   * @param buffer Buffer to release
   * @param size Original buffer size
   */
  void release(std::unique_ptr<uint8_t[]> buffer, size_t size);

  /**
   * @brief Get statistics for all pools
   * @return Combined statistics
   */
  MemoryPool::PoolStats get_stats() const;

  /**
   * @brief Get hit rate for all pools
   * @return Combined hit rate
   */
  double get_hit_rate() const;

  /**
   * @brief Get memory usage for all pools
   * @return Combined memory usage
   */
  std::pair<size_t, size_t> get_memory_usage() const;

  /**
   * @brief Get statistics for a specific size category
   * @param category Size category
   * @return Statistics for the category
   */
  MemoryPool::PoolStats get_stats(SizeCategory category) const;

  /**
   * @brief Get hit rate for a specific size category
   * @param category Size category
   * @return Hit rate for the category
   */
  double get_hit_rate(SizeCategory category) const;

  /**
   * @brief Cleanup old buffers in all pools
   * @param max_age Maximum age for buffers
   */
  void cleanup_old_buffers(std::chrono::milliseconds max_age = std::chrono::minutes(5));

  /**
   * @brief Resize all pools
   * @param new_size New size for all pools
   */
  void resize_pool(size_t new_size);

  /**
   * @brief Auto-tune all pools based on usage patterns
   */
  void auto_tune();

  /**
   * @brief Get health metrics for all pools
   * @return Combined health metrics
   */
  MemoryPool::HealthMetrics get_health_metrics() const;

 public:
  // Determine size category for a given buffer size (public for testing)
  SizeCategory get_size_category(size_t size) const;

 private:
  // Get the appropriate pool for a size category
  MemoryPool& get_pool(SizeCategory category);
  const MemoryPool& get_pool(SizeCategory category) const;

  // Individual pools for different size categories
  std::unique_ptr<MemoryPool> small_pool_;   // 1KB - 4KB
  std::unique_ptr<MemoryPool> medium_pool_;  // 8KB - 32KB
  std::unique_ptr<MemoryPool> large_pool_;   // 64KB+

  // Size thresholds
  static constexpr size_t SMALL_THRESHOLD = 4096;    // 4KB
  static constexpr size_t MEDIUM_THRESHOLD = 32768;  // 32KB
};

/**
 * @brief RAII wrapper for optimized memory pool buffers
 */
class OptimizedPooledBuffer {
 public:
  explicit OptimizedPooledBuffer(size_t size, OptimizedMemoryPool& pool);
  ~OptimizedPooledBuffer();

  // Non-copyable, movable
  OptimizedPooledBuffer(const OptimizedPooledBuffer&) = delete;
  OptimizedPooledBuffer& operator=(const OptimizedPooledBuffer&) = delete;
  OptimizedPooledBuffer(OptimizedPooledBuffer&& other) noexcept;
  OptimizedPooledBuffer& operator=(OptimizedPooledBuffer&& other) noexcept;

  uint8_t* data() const;
  size_t size() const;
  bool valid() const;

 private:
  std::unique_ptr<uint8_t[]> buffer_;
  size_t size_;
  OptimizedMemoryPool& pool_;
};

/**
 * @brief Global optimized memory pool instance
 */
class GlobalOptimizedMemoryPool {
 public:
  static OptimizedMemoryPool& instance() {
    static OptimizedMemoryPool pool;
    return pool;
  }

 private:
  GlobalOptimizedMemoryPool() = delete;
  GlobalOptimizedMemoryPool(const GlobalOptimizedMemoryPool&) = delete;
  GlobalOptimizedMemoryPool& operator=(const GlobalOptimizedMemoryPool&) = delete;
};

}  // namespace unilink::common
