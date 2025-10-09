#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

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

  // Health monitoring structures
  struct HealthMetrics {
    // Real-time health indicators
    double memory_utilization{0.0};   // Current memory usage percentage
    double allocation_rate{0.0};      // Allocations per second
    double hit_rate{0.0};             // Current hit rate
    double average_latency_ms{0.0};   // Average allocation latency
    size_t active_buffers{0};         // Currently active buffers
    size_t free_buffers{0};           // Available buffers
    double fragmentation_ratio{0.0};  // Memory fragmentation level

    // Performance trends
    double hit_rate_trend{0.0};      // Hit rate trend (positive = improving)
    double latency_trend{0.0};       // Latency trend (negative = improving)
    double memory_growth_rate{0.0};  // Memory growth rate per second

    // Health status
    enum class HealthStatus {
      EXCELLENT,  // All metrics optimal
      GOOD,       // Minor issues, still performing well
      WARNING,    // Some metrics concerning
      CRITICAL,   // Performance degradation
      FAILURE     // System failure or severe issues
    } status{HealthStatus::EXCELLENT};

    // Timestamp for metrics
    std::chrono::steady_clock::time_point timestamp;

    // Alert conditions
    bool memory_pressure{false};          // High memory usage
    bool performance_degradation{false};  // Performance issues
    bool fragmentation_high{false};       // High fragmentation
    bool hit_rate_low{false};             // Low hit rate
  };

  struct AlertThresholds {
    // Memory thresholds
    double memory_utilization_warning{0.8};    // 80% memory usage
    double memory_utilization_critical{0.95};  // 95% memory usage

    // Performance thresholds
    double hit_rate_warning{0.1};     // 10% hit rate
    double hit_rate_critical{0.05};   // 5% hit rate
    double latency_warning_ms{1.0};   // 1ms latency
    double latency_critical_ms{5.0};  // 5ms latency

    // Fragmentation thresholds
    double fragmentation_warning{0.3};   // 30% fragmentation
    double fragmentation_critical{0.5};  // 50% fragmentation

    // Rate thresholds
    double allocation_rate_warning{1000.0};   // 1000 allocs/sec
    double allocation_rate_critical{5000.0};  // 5000 allocs/sec
  };

  // Cache line aligned structure for optimal memory access
  struct alignas(64) BufferInfo {
    std::unique_ptr<uint8_t[]> data;
    size_t size;
    std::chrono::steady_clock::time_point last_used;
    bool in_use{false};

    // Free list pointer for O(1) allocation
    BufferInfo* next_free{nullptr};

    // Lock-free operation support
    std::atomic<bool> lock_free_in_use{false};
    std::atomic<BufferInfo*> lock_free_next{nullptr};

    // Default constructor
    BufferInfo() = default;

    // Move constructor
    BufferInfo(BufferInfo&& other) noexcept
        : data(std::move(other.data)),
          size(other.size),
          last_used(other.last_used),
          in_use(other.in_use),
          next_free(other.next_free),
          lock_free_in_use(other.lock_free_in_use.load()),
          lock_free_next(other.lock_free_next.load()) {
      other.next_free = nullptr;
      other.lock_free_next.store(nullptr);
    }

    // Move assignment
    BufferInfo& operator=(BufferInfo&& other) noexcept {
      if (this != &other) {
        data = std::move(other.data);
        size = other.size;
        last_used = other.last_used;
        in_use = other.in_use;
        next_free = other.next_free;
        lock_free_in_use.store(other.lock_free_in_use.load());
        lock_free_next.store(other.lock_free_next.load());
        other.next_free = nullptr;
        other.lock_free_next.store(nullptr);
      }
      return *this;
    }

    // Delete copy operations
    BufferInfo(const BufferInfo&) = delete;
    BufferInfo& operator=(const BufferInfo&) = delete;
  };

  // Predefined buffer sizes for common use cases
  enum class BufferSize : size_t {
    SMALL = 1024,   // 1KB - small messages
    MEDIUM = 4096,  // 4KB - typical network packets
    LARGE = 16384,  // 16KB - large data transfers
    XLARGE = 65536  // 64KB - bulk operations
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
   * @param size Required buffer size (must be > 0 and <= MAX_BUFFER_SIZE)
   * @return std::unique_ptr<uint8_t[]> Buffer or nullptr if allocation fails
   * @throws std::invalid_argument if size is invalid
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
   * @param buffer Buffer to release (can be nullptr)
   * @param size Size of the buffer (must match the buffer's actual size)
   * @throws std::invalid_argument if size is invalid
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

  // Health monitoring interface
  /**
   * @brief Get current health status of the memory pool
   * @return HealthMetrics with current health indicators
   */
  HealthMetrics get_health_status() const;

  /**
   * @brief Get active alerts
   * @return Vector of active alert messages
   */
  std::vector<std::string> get_active_alerts() const;

  /**
   * @brief Set alert thresholds
   * @param thresholds New alert thresholds
   */
  void set_alert_thresholds(const AlertThresholds& thresholds);

  /**
   * @brief Enable or disable alerts
   * @param enable True to enable alerts, false to disable
   */
  void enable_alerts(bool enable = true);

  /**
   * @brief Disable alerts
   */
  void disable_alerts();

  /**
   * @brief Get health metrics history
   * @param max_entries Maximum number of entries to return
   * @return Vector of historical health metrics
   */
  std::vector<HealthMetrics> get_health_history(size_t max_entries = 10) const;

 private:
  // Cache line aligned bucket for optimal performance
  struct alignas(64) PoolBucket {
    std::vector<BufferInfo> buffers;
    mutable std::mutex mutex;
    size_t size;
    std::atomic<size_t> hits{0};
    std::atomic<size_t> misses{0};

    // Free list management for O(1) allocation
    BufferInfo* free_list_head{nullptr};
    std::atomic<size_t> free_count{0};

    // Lock-free free list management
    std::atomic<BufferInfo*> lock_free_free_list_head{nullptr};
    std::atomic<size_t> lock_free_free_count{0};

    // Lock-free buffer pool for better memory efficiency
    std::vector<std::unique_ptr<uint8_t[]>> lock_free_buffer_pool;
    std::atomic<size_t> lock_free_pool_index{0};
    std::atomic<size_t> lock_free_pool_allocated{0};

    // Lazy cleanup optimization
    std::vector<size_t> expired_indices;  // Indices of expired buffers to remove
    std::chrono::steady_clock::time_point last_cleanup_time;
    static constexpr std::chrono::milliseconds CLEANUP_INTERVAL{100};  // 100ms cleanup interval

    // Adaptive algorithm selection
    enum class CleanupAlgorithm {
      LAZY_OPTIMIZED,    // New algorithm for low expiration ratio
      TRADITIONAL_ERASE  // Traditional algorithm for high expiration ratio
    };

    CleanupAlgorithm current_cleanup_algorithm{CleanupAlgorithm::LAZY_OPTIMIZED};
    double last_expiration_ratio{0.0};
    size_t cleanup_performance_samples{0};
    std::chrono::microseconds total_cleanup_time{0};
    static constexpr double EXPIRATION_RATIO_THRESHOLD{0.3};  // 30% threshold
    static constexpr size_t ALGORITHM_SWITCH_SAMPLES{10};     // Switch after 10 samples

    // Default constructor
    PoolBucket() = default;

    // Move constructor
    PoolBucket(PoolBucket&& other) noexcept
        : buffers(std::move(other.buffers)),
          size(other.size),
          hits(other.hits.load()),
          misses(other.misses.load()),
          free_list_head(other.free_list_head),
          free_count(other.free_count.load()) {
      other.free_list_head = nullptr;
    }

    // Move assignment
    PoolBucket& operator=(PoolBucket&& other) noexcept {
      if (this != &other) {
        buffers = std::move(other.buffers);
        size = other.size;
        hits.store(other.hits.load());
        misses.store(other.misses.load());
        free_list_head = other.free_list_head;
        free_count.store(other.free_count.load());
        other.free_list_head = nullptr;
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

  // Batch statistics update for lock contention reduction
  struct LocalStats {
    size_t pool_hits{0};
    size_t pool_misses{0};
    size_t total_allocations{0};
    std::chrono::milliseconds::rep total_allocation_time{0};
    std::chrono::milliseconds::rep total_deallocation_time{0};
    std::unordered_map<size_t, size_t> hits_by_size;
    std::unordered_map<size_t, size_t> misses_by_size;
    std::unordered_map<size_t, size_t> usage_count;
  };

  // Health monitoring data
  struct HealthMonitoringData {
    // Historical metrics for trend analysis
    std::vector<HealthMetrics> metrics_history;
    static constexpr size_t MAX_HISTORY_SIZE = 100;  // Keep last 100 measurements

    // Alert thresholds
    AlertThresholds thresholds;

    // Monitoring state
    std::chrono::steady_clock::time_point last_health_check;
    std::chrono::steady_clock::time_point last_metrics_update;
    static constexpr std::chrono::milliseconds HEALTH_CHECK_INTERVAL{1000};   // 1 second
    static constexpr std::chrono::milliseconds METRICS_UPDATE_INTERVAL{100};  // 100ms

    // Performance counters for rate calculations (atomic to avoid locks)
    std::atomic<size_t> allocations_since_last_check{0};
    std::atomic<size_t> memory_allocated_since_last_check{0};
    std::chrono::steady_clock::time_point last_rate_calculation;

    // Alert state
    bool alerts_enabled{true};
    std::vector<std::string> active_alerts;
  };

  thread_local static LocalStats local_stats_;
  std::atomic<size_t> batch_update_counter_{0};
  static constexpr size_t BATCH_UPDATE_THRESHOLD = 100;

  // Health monitoring
  mutable HealthMonitoringData health_monitoring_;
  mutable std::mutex health_mutex_;

  // Batch update methods
  void flush_local_stats();
  void update_stats_from_local(const LocalStats& local);

  // Predefined bucket sizes - sorted for binary search
  static constexpr std::array<size_t, 4> BUCKET_SIZES = {
      static_cast<size_t>(BufferSize::SMALL), static_cast<size_t>(BufferSize::MEDIUM),
      static_cast<size_t>(BufferSize::LARGE), static_cast<size_t>(BufferSize::XLARGE)};

 public:
  // Safety constants
  static constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024 * 1024;  // 1GB
  static constexpr size_t MIN_BUFFER_SIZE = 1;
  static constexpr size_t MAX_POOL_SIZE = 10000;

  // Memory alignment constants for optimal cache performance
  static constexpr size_t CACHE_LINE_SIZE = 64;
  static constexpr size_t ALIGNMENT_SIZE = 64;
  static constexpr size_t ALIGNMENT_THRESHOLD = 4096;  // Only align buffers >= 4KB

  // Lock-free operation constants
  static constexpr size_t LOCK_FREE_THRESHOLD = 1000;  // Use lock-free for pools >= 1000 buffers
  static constexpr size_t MAX_LOCK_FREE_RETRIES = 10;  // Maximum retries for lock-free operations
  static constexpr size_t LOCK_FREE_BATCH_SIZE = 8;    // Batch size for lock-free operations
  static constexpr size_t LOCK_FREE_POOL_SIZE = 100;   // Size of lock-free buffer pool per bucket

 private:
  // Helper function for binary search
  size_t find_bucket_index(size_t size) const;

  // Input validation functions
  void validate_size(size_t size) const;
  void validate_pool_size(size_t size) const;

  // Free list management functions
  void add_to_free_list(PoolBucket& bucket, BufferInfo* buffer_info);
  BufferInfo* remove_from_free_list(PoolBucket& bucket);
  void rebuild_free_list(PoolBucket& bucket);

  // Memory alignment optimization functions
  size_t align_size(size_t size) const;
  std::unique_ptr<uint8_t[]> create_aligned_buffer(size_t size);

  // Optimized cleanup functions
  void lazy_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age);
  void perform_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age);
  void remove_expired_buffers_efficiently(PoolBucket& bucket, const std::vector<size_t>& expired_indices);

  // Adaptive algorithm selection functions
  void adaptive_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age);
  void traditional_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age);
  void select_optimal_cleanup_algorithm(PoolBucket& bucket, double expiration_ratio,
                                        std::chrono::microseconds cleanup_time);
  bool should_use_aligned_allocation(size_t size) const;

  // Lock-free operation functions
  bool should_use_lock_free_operations(size_t pool_size) const;
  std::unique_ptr<uint8_t[]> acquire_lock_free(size_t size);
  void release_lock_free(std::unique_ptr<uint8_t[]> buffer, size_t size);
  BufferInfo* remove_from_lock_free_free_list(PoolBucket& bucket);
  void add_to_lock_free_free_list(PoolBucket& bucket, BufferInfo* buffer_info);
  void rebuild_lock_free_free_list(PoolBucket& bucket);

  // Improved lock-free memory management
  void initialize_lock_free_pool(PoolBucket& bucket);
  std::unique_ptr<uint8_t[]> acquire_from_lock_free_pool(PoolBucket& bucket);
  void return_to_lock_free_pool(PoolBucket& bucket, std::unique_ptr<uint8_t[]> buffer);
  bool is_lock_free_pool_available(PoolBucket& bucket) const;

  // Health monitoring functions
  HealthMetrics get_current_health_metrics() const;
  void update_health_metrics();
  void check_health_status();
  void add_health_metrics_to_history(const HealthMetrics& metrics);
  double calculate_fragmentation_ratio() const;
  double calculate_hit_rate_trend() const;
  double calculate_latency_trend() const;
  double calculate_memory_growth_rate() const;
  HealthMetrics::HealthStatus determine_health_status(const HealthMetrics& metrics) const;
  void generate_alerts(const HealthMetrics& metrics);
  void clear_resolved_alerts();
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
 * @brief RAII wrapper for memory pool buffers with enhanced safety
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

  // Safe access methods
  uint8_t* data() const { return buffer_.get(); }
  size_t size() const { return size_; }
  bool valid() const { return buffer_ != nullptr; }

  // Safe array access with bounds checking
  uint8_t& operator[](size_t index);
  const uint8_t& operator[](size_t index) const;

  // Safe pointer arithmetic
  uint8_t* at(size_t offset) const;

  // Explicit conversion methods (no implicit conversion)
  uint8_t* get() const { return data(); }
  explicit operator bool() const { return valid(); }

 private:
  std::unique_ptr<uint8_t[]> buffer_;
  size_t size_;
  MemoryPool* pool_;

  // Helper for bounds checking
  void check_bounds(size_t index) const;
};

}  // namespace common
}  // namespace unilink
