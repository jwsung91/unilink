#include "unilink/common/memory_pool.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>  // for std::aligned_alloc and std::free
#include <iostream>
#include <stdexcept>
#include <string>

#include "unilink/common/constants.hpp"

namespace unilink {
namespace common {

// Thread-local storage for batch statistics
thread_local MemoryPool::LocalStats MemoryPool::local_stats_;

// ============================================================================
// EnhancedMemoryPool Implementation
// ============================================================================

MemoryPool::MemoryPool(size_t initial_pool_size, size_t max_pool_size) : max_pool_size_(max_pool_size) {
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
  // Input validation
  validate_size(size);

  // Use lock-free operations for large pools
  if (should_use_lock_free_operations(max_pool_size_)) {
    return acquire_lock_free(size);
  }

  auto start_time = std::chrono::steady_clock::now();

  // Track usage for auto-tuning (local, no lock)
  local_stats_.usage_count[size]++;

  // Check if we need to flush local stats
  if (++batch_update_counter_ >= BATCH_UPDATE_THRESHOLD) {
    flush_local_stats();
  }

  PoolBucket& bucket = get_bucket(size);
  std::lock_guard<std::mutex> lock(bucket.mutex);

  // Try to get a buffer from the free list (O(1) operation)
  BufferInfo* buffer_info = remove_from_free_list(bucket);

  if (buffer_info != nullptr) {
    // Found an available buffer from free list
    buffer_info->in_use = true;
    buffer_info->last_used = std::chrono::steady_clock::now();
    bucket.hits++;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Update local statistics (no lock)
    local_stats_.pool_hits++;
    local_stats_.total_allocations++;
    local_stats_.total_allocation_time += duration.count();
    local_stats_.hits_by_size[size]++;

    // Update health monitoring counters (atomic, no lock)
    health_monitoring_.allocations_since_last_check.fetch_add(1);
    health_monitoring_.memory_allocated_since_last_check.fetch_add(bucket.size);

    return std::move(buffer_info->data);
  } else {
    // Pool miss - create new buffer
    bucket.misses++;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Update local statistics (no lock)
    local_stats_.pool_misses++;
    local_stats_.total_allocations++;
    local_stats_.total_allocation_time += duration.count();
    local_stats_.misses_by_size[size]++;

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
  // Input validation
  validate_size(size);

  if (!buffer) return;

  // Use lock-free operations for large pools
  if (should_use_lock_free_operations(max_pool_size_)) {
    release_lock_free(std::move(buffer), size);
    return;
  }

  auto start_time = std::chrono::steady_clock::now();

  PoolBucket& bucket = get_bucket(size);
  std::lock_guard<std::mutex> lock(bucket.mutex);

  // Check if we can add this buffer back to the pool
  if (bucket.buffers.size() < max_pool_size_ / BUCKET_SIZES.size()) {
    // Create new BufferInfo and add to pool
    bucket.buffers.emplace_back();
    BufferInfo& info = bucket.buffers.back();
    info.data = std::move(buffer);
    info.size = bucket.size;
    info.last_used = std::chrono::steady_clock::now();
    info.in_use = false;
    info.next_free = nullptr;

    // Add to free list for O(1) future allocation
    add_to_free_list(bucket, &info);
    current_total_buffers_++;

    // Note: Memory tracking is not updated here because the buffer is being
    // returned to the pool, not actually deallocated. The memory remains allocated.
  }
  // If pool is full, let the buffer be destroyed automatically

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Update local deallocation time (no lock)
  local_stats_.total_deallocation_time += duration.count();
}

void MemoryPool::flush_local_stats() {
  if (local_stats_.total_allocations == 0) return;  // Nothing to flush

  std::lock_guard<std::mutex> stats_lock(stats_mutex_);
  update_stats_from_local(local_stats_);

  // Reset local stats
  local_stats_ = LocalStats{};
  batch_update_counter_ = 0;
}

void MemoryPool::update_stats_from_local(const LocalStats& local) {
  stats_.pool_hits += local.pool_hits;
  stats_.pool_misses += local.pool_misses;
  stats_.total_allocations += local.total_allocations;
  stats_.total_allocation_time += local.total_allocation_time;
  stats_.total_deallocation_time += local.total_deallocation_time;

  // Update size-specific statistics
  for (const auto& [size, count] : local.hits_by_size) {
    stats_.hits_by_size[size] += count;
  }
  for (const auto& [size, count] : local.misses_by_size) {
    stats_.misses_by_size[size] += count;
  }

  // Update usage count for auto-tuning
  for (const auto& [size, count] : local.usage_count) {
    size_usage_count_[size] += count;
  }
}

MemoryPool::PoolStats MemoryPool::get_stats() const {
  // Flush any pending local stats before reading
  const_cast<MemoryPool*>(this)->flush_local_stats();

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
  // Input validation
  validate_pool_size(new_size);

  max_pool_size_ = new_size;

  {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.max_pool_size = new_size;
  }
}

void MemoryPool::cleanup_old_buffers(std::chrono::milliseconds max_age) {
  // Use lazy cleanup for better performance
  for (auto& bucket : buckets_) {
    lazy_cleanup_bucket(bucket, max_age);
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
  size_t index = find_bucket_index(size);
  return buckets_[index];
}

const MemoryPool::PoolBucket& MemoryPool::get_bucket(size_t size) const {
  size_t index = find_bucket_index(size);
  return buckets_[index];
}

size_t MemoryPool::find_bucket_index(size_t size) const {
  // Binary search for the smallest bucket that can accommodate the requested size
  size_t left = 0;
  size_t right = BUCKET_SIZES.size();

  while (left < right) {
    size_t mid = left + (right - left) / 2;
    if (BUCKET_SIZES[mid] < size) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  // If no bucket is large enough, return the largest one
  if (left >= BUCKET_SIZES.size()) {
    return BUCKET_SIZES.size() - 1;
  }

  return left;
}

void MemoryPool::validate_size(size_t size) const {
  if (size < MIN_BUFFER_SIZE) {
    throw std::invalid_argument("Buffer size must be at least " + std::to_string(MIN_BUFFER_SIZE) + " bytes");
  }
  if (size > MAX_BUFFER_SIZE) {
    throw std::invalid_argument("Buffer size cannot exceed " + std::to_string(MAX_BUFFER_SIZE) + " bytes");
  }
}

void MemoryPool::validate_pool_size(size_t size) const {
  if (size == 0) {
    throw std::invalid_argument("Pool size must be greater than 0");
  }
  if (size > MAX_POOL_SIZE) {
    throw std::invalid_argument("Pool size cannot exceed " + std::to_string(MAX_POOL_SIZE));
  }
}

std::unique_ptr<uint8_t[]> MemoryPool::create_buffer(size_t size) { return create_aligned_buffer(size); }

std::unique_ptr<uint8_t[]> MemoryPool::create_aligned_buffer(size_t size) {
  try {
    // Adaptive alignment: only align large buffers to avoid overhead
    if (should_use_aligned_allocation(size)) {
      size_t aligned_size = align_size(size);
      return std::make_unique<uint8_t[]>(aligned_size);
    } else {
      // Use regular allocation for small buffers
      return std::make_unique<uint8_t[]>(size);
    }
  } catch (const std::bad_alloc& e) {
    // Log allocation failure for debugging
    std::cerr << "Memory allocation failed for size " << size << ": " << e.what() << std::endl;
    return nullptr;
  }
}

size_t MemoryPool::align_size(size_t size) const {
  // Align size to cache line boundary
  return (size + ALIGNMENT_SIZE - 1) & ~(ALIGNMENT_SIZE - 1);
}

void MemoryPool::cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age) {
  adaptive_cleanup_bucket(bucket, max_age);
}

void MemoryPool::lazy_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age) {
  std::lock_guard<std::mutex> lock(bucket.mutex);

  auto now = std::chrono::steady_clock::now();

  // Check if enough time has passed since last cleanup
  if (now - bucket.last_cleanup_time < bucket.CLEANUP_INTERVAL) {
    return;  // Skip cleanup if too recent
  }

  // Perform the actual cleanup
  perform_cleanup_bucket(bucket, max_age);
  bucket.last_cleanup_time = now;
}

void MemoryPool::perform_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age) {
  auto now = std::chrono::steady_clock::now();
  auto cutoff_time = now - max_age;

  // Find expired buffer indices (more efficient than remove_if)
  bucket.expired_indices.clear();
  bucket.expired_indices.reserve(bucket.buffers.size() / 4);  // Reserve space for potential removals

  for (size_t i = 0; i < bucket.buffers.size(); ++i) {
    const auto& info = bucket.buffers[i];
    if (!info.in_use && info.last_used < cutoff_time) {
      bucket.expired_indices.push_back(i);
    }
  }

  // Remove expired buffers efficiently
  if (!bucket.expired_indices.empty()) {
    remove_expired_buffers_efficiently(bucket, bucket.expired_indices);
  }

  // Rebuild free list after cleanup to maintain consistency
  rebuild_free_list(bucket);
}

void MemoryPool::remove_expired_buffers_efficiently(PoolBucket& bucket, const std::vector<size_t>& expired_indices) {
  if (expired_indices.empty()) return;

  // Sort indices in descending order to remove from end first (avoids index shifting)
  std::vector<size_t> sorted_indices = expired_indices;
  std::sort(sorted_indices.begin(), sorted_indices.end(), std::greater<size_t>());

  // Remove buffers from highest index to lowest (O(k) where k = number of expired buffers)
  for (size_t index : sorted_indices) {
    if (index < bucket.buffers.size()) {
      // Move the last element to the expired position (O(1) operation)
      if (index != bucket.buffers.size() - 1) {
        bucket.buffers[index] = std::move(bucket.buffers.back());
      }
      bucket.buffers.pop_back();
    }
  }
}

void MemoryPool::adaptive_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age) {
  std::lock_guard<std::mutex> lock(bucket.mutex);

  auto now = std::chrono::steady_clock::now();

  // Check if enough time has passed since last cleanup
  if (now - bucket.last_cleanup_time < bucket.CLEANUP_INTERVAL) {
    return;  // Skip cleanup if too recent
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  // Calculate expiration ratio
  auto cutoff_time = now - max_age;
  size_t expired_count = 0;
  for (const auto& info : bucket.buffers) {
    if (!info.in_use && info.last_used < cutoff_time) {
      expired_count++;
    }
  }

  double expiration_ratio = bucket.buffers.empty() ? 0.0 : static_cast<double>(expired_count) / bucket.buffers.size();

  // Select optimal algorithm based on expiration ratio
  if (expiration_ratio < bucket.EXPIRATION_RATIO_THRESHOLD) {
    // Low expiration ratio: use optimized algorithm
    bucket.current_cleanup_algorithm = PoolBucket::CleanupAlgorithm::LAZY_OPTIMIZED;
    perform_cleanup_bucket(bucket, max_age);
  } else {
    // High expiration ratio: use traditional algorithm
    bucket.current_cleanup_algorithm = PoolBucket::CleanupAlgorithm::TRADITIONAL_ERASE;
    traditional_cleanup_bucket(bucket, max_age);
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto cleanup_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  // Update performance tracking
  bucket.total_cleanup_time += cleanup_time;
  bucket.cleanup_performance_samples++;
  bucket.last_expiration_ratio = expiration_ratio;

  bucket.last_cleanup_time = now;
}

void MemoryPool::traditional_cleanup_bucket(PoolBucket& bucket, std::chrono::milliseconds max_age) {
  auto now = std::chrono::steady_clock::now();
  auto cutoff_time = now - max_age;

  // Use traditional remove_if + erase for high expiration ratios
  bucket.buffers.erase(
      std::remove_if(bucket.buffers.begin(), bucket.buffers.end(),
                     [&cutoff_time](const BufferInfo& info) { return !info.in_use && info.last_used < cutoff_time; }),
      bucket.buffers.end());

  // Rebuild free list after cleanup
  rebuild_free_list(bucket);
}

void MemoryPool::select_optimal_cleanup_algorithm(PoolBucket& bucket, double expiration_ratio,
                                                  std::chrono::microseconds cleanup_time) {
  // This function can be used for future machine learning-based algorithm selection
  // For now, we use simple threshold-based selection in adaptive_cleanup_bucket

  // Track performance metrics for future optimization
  if (bucket.cleanup_performance_samples >= bucket.ALGORITHM_SWITCH_SAMPLES) {
    // Reset performance tracking
    bucket.cleanup_performance_samples = 0;
    bucket.total_cleanup_time = std::chrono::microseconds{0};
  }
}

bool MemoryPool::should_use_aligned_allocation(size_t size) const {
  // Only use aligned allocation for buffers >= ALIGNMENT_THRESHOLD
  // This avoids excessive memory overhead for small buffers
  return size >= ALIGNMENT_THRESHOLD;
}

bool MemoryPool::should_use_lock_free_operations(size_t pool_size) const {
  // Use lock-free operations for large pools where lock contention is high
  return pool_size >= LOCK_FREE_THRESHOLD;
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire_lock_free(size_t size) {
  // Input validation
  validate_size(size);

  auto start_time = std::chrono::steady_clock::now();

  // Track usage for auto-tuning (local, no lock)
  local_stats_.usage_count[size]++;

  // Check if we need to flush local stats
  if (++batch_update_counter_ >= BATCH_UPDATE_THRESHOLD) {
    flush_local_stats();
  }

  PoolBucket& bucket = get_bucket(size);

  // Initialize lock-free pool if not already done
  if (bucket.lock_free_buffer_pool.empty()) {
    initialize_lock_free_pool(bucket);
  }

  // Try to get buffer from lock-free pool first
  auto buffer = acquire_from_lock_free_pool(bucket);

  if (buffer) {
    // Successfully got buffer from pool (hit)
    bucket.hits++;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Update local statistics (no lock)
    local_stats_.pool_hits++;
    local_stats_.total_allocations++;
    local_stats_.total_allocation_time += duration.count();
    local_stats_.hits_by_size[size]++;

    return buffer;
  } else {
    // Pool miss - create new buffer
    bucket.misses++;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Update local statistics (no lock)
    local_stats_.pool_misses++;
    local_stats_.total_allocations++;
    local_stats_.total_allocation_time += duration.count();
    local_stats_.misses_by_size[size]++;

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

void MemoryPool::release_lock_free(std::unique_ptr<uint8_t[]> buffer, size_t size) {
  // Input validation
  validate_size(size);

  if (!buffer) return;

  auto start_time = std::chrono::steady_clock::now();

  PoolBucket& bucket = get_bucket(size);

  // Try to return buffer to lock-free pool for reuse
  if (is_lock_free_pool_available(bucket)) {
    return_to_lock_free_pool(bucket, std::move(buffer));
  }
  // If pool is full, let the buffer be destroyed automatically

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Update local deallocation time (no lock)
  local_stats_.total_deallocation_time += duration.count();
}

MemoryPool::BufferInfo* MemoryPool::remove_from_lock_free_free_list(PoolBucket& bucket) {
  // Lock-free removal using compare-and-swap
  for (size_t retry = 0; retry < MAX_LOCK_FREE_RETRIES; ++retry) {
    BufferInfo* current_head = bucket.lock_free_free_list_head.load(std::memory_order_acquire);

    if (current_head == nullptr) {
      return nullptr;  // No free buffers available
    }

    // Try to atomically remove the head
    BufferInfo* next = current_head->lock_free_next.load(std::memory_order_relaxed);

    if (bucket.lock_free_free_list_head.compare_exchange_weak(current_head, next, std::memory_order_release,
                                                              std::memory_order_acquire)) {
      // Successfully removed the head
      current_head->lock_free_next.store(nullptr, std::memory_order_relaxed);
      bucket.lock_free_free_count.fetch_sub(1, std::memory_order_relaxed);
      return current_head;
    }

    // CAS failed, retry
    // Yield to other threads (simple busy wait)
    for (volatile int i = 0; i < common::constants::DEFAULT_MEMORY_POOL_SIZE; ++i) {
    }
  }

  return nullptr;  // Failed after max retries
}

void MemoryPool::add_to_lock_free_free_list(PoolBucket& bucket, MemoryPool::BufferInfo* buffer_info) {
  if (buffer_info == nullptr) return;

  // Lock-free addition using compare-and-swap
  for (size_t retry = 0; retry < MAX_LOCK_FREE_RETRIES; ++retry) {
    BufferInfo* current_head = bucket.lock_free_free_list_head.load(std::memory_order_acquire);
    buffer_info->lock_free_next.store(current_head, std::memory_order_relaxed);

    if (bucket.lock_free_free_list_head.compare_exchange_weak(current_head, buffer_info, std::memory_order_release,
                                                              std::memory_order_acquire)) {
      // Successfully added to head
      bucket.lock_free_free_count.fetch_add(1, std::memory_order_relaxed);
      return;
    }

    // CAS failed, retry
    // Yield to other threads (simple busy wait)
    for (volatile int i = 0; i < common::constants::DEFAULT_MEMORY_POOL_SIZE; ++i) {
    }
  }
}

void MemoryPool::rebuild_lock_free_free_list(PoolBucket& bucket) {
  // Clear existing lock-free free list
  bucket.lock_free_free_list_head.store(nullptr, std::memory_order_relaxed);
  bucket.lock_free_free_count.store(0, std::memory_order_relaxed);

  // Rebuild lock-free free list from unused buffers
  for (auto& buffer_info : bucket.buffers) {
    if (!buffer_info.in_use && !buffer_info.lock_free_in_use.load(std::memory_order_relaxed)) {
      add_to_lock_free_free_list(bucket, &buffer_info);
    }
  }
}

void MemoryPool::initialize_lock_free_pool(PoolBucket& bucket) {
  // Initialize lock-free buffer pool with pre-allocated buffers
  bucket.lock_free_buffer_pool.reserve(LOCK_FREE_POOL_SIZE);

  for (size_t i = 0; i < LOCK_FREE_POOL_SIZE; ++i) {
    auto buffer = create_buffer(bucket.size);
    if (buffer) {
      bucket.lock_free_buffer_pool.push_back(std::move(buffer));
    }
  }

  bucket.lock_free_pool_index.store(0, std::memory_order_relaxed);
  bucket.lock_free_pool_allocated.store(0, std::memory_order_relaxed);
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire_from_lock_free_pool(PoolBucket& bucket) {
  // Lock-free acquisition from pre-allocated pool
  size_t current_index = bucket.lock_free_pool_index.load(std::memory_order_acquire);
  size_t allocated_count = bucket.lock_free_pool_allocated.load(std::memory_order_acquire);

  // Check if pool has available buffers
  if (allocated_count >= bucket.lock_free_buffer_pool.size() || bucket.lock_free_buffer_pool.empty()) {
    return nullptr;  // Pool exhausted or not initialized
  }

  // Try to atomically allocate a buffer
  size_t next_index = (current_index + 1) % bucket.lock_free_buffer_pool.size();

  if (bucket.lock_free_pool_index.compare_exchange_weak(current_index, next_index, std::memory_order_release,
                                                        std::memory_order_acquire)) {
    // Successfully allocated buffer
    bucket.lock_free_pool_allocated.fetch_add(1, std::memory_order_relaxed);

    // Move buffer out of pool (with bounds checking)
    if (current_index < bucket.lock_free_buffer_pool.size()) {
      std::unique_ptr<uint8_t[]> buffer = std::move(bucket.lock_free_buffer_pool[current_index]);
      bucket.lock_free_buffer_pool[current_index] = nullptr;  // Mark as allocated

      return buffer;
    }
  }

  return nullptr;  // Failed to allocate
}

void MemoryPool::return_to_lock_free_pool(PoolBucket& bucket, std::unique_ptr<uint8_t[]> buffer) {
  if (!buffer || bucket.lock_free_buffer_pool.empty()) return;

  // Find an empty slot in the pool
  for (size_t i = 0; i < bucket.lock_free_buffer_pool.size(); ++i) {
    if (bucket.lock_free_buffer_pool[i] == nullptr) {
      // Found empty slot, return buffer
      bucket.lock_free_buffer_pool[i] = std::move(buffer);
      bucket.lock_free_pool_allocated.fetch_sub(1, std::memory_order_relaxed);
      return;
    }
  }

  // No empty slot found, buffer will be destroyed
}

bool MemoryPool::is_lock_free_pool_available(PoolBucket& bucket) const {
  size_t allocated_count = bucket.lock_free_pool_allocated.load(std::memory_order_relaxed);
  return allocated_count < bucket.lock_free_buffer_pool.size();
}

// ============================================================================
// Health Monitoring Implementation
// ============================================================================

MemoryPool::HealthMetrics MemoryPool::get_current_health_metrics() const {
  HealthMetrics metrics;
  auto now = std::chrono::steady_clock::now();

  // Get current statistics without acquiring additional locks
  // Use atomic values directly to avoid deadlock
  metrics.memory_utilization = (total_memory_allocated_ > 0)
                                   ? static_cast<double>(total_memory_allocated_) / (total_memory_allocated_ + 1)
                                   : 0.0;

  // Calculate hit rate from atomic counters
  size_t total_ops = stats_.total_allocations;
  metrics.hit_rate = (total_ops > 0) ? static_cast<double>(stats_.pool_hits) / total_ops : 0.0;

  // Use atomic values for latency
  metrics.average_latency_ms = (total_ops > 0) ? static_cast<double>(stats_.total_allocation_time) / total_ops : 0.0;

  metrics.active_buffers = current_total_buffers_;
  metrics.free_buffers = max_pool_size_ - current_total_buffers_;
  metrics.fragmentation_ratio = calculate_fragmentation_ratio();

  // Calculate rates
  auto time_since_last_check = now - health_monitoring_.last_rate_calculation;
  if (time_since_last_check > std::chrono::milliseconds(common::constants::DEFAULT_CLEANUP_INTERVAL_MS)) {
    double seconds = std::chrono::duration<double>(time_since_last_check).count();
    metrics.allocation_rate = health_monitoring_.allocations_since_last_check.load() / seconds;
    metrics.memory_growth_rate = health_monitoring_.memory_allocated_since_last_check.load() / seconds;

    // Reset counters
    health_monitoring_.allocations_since_last_check.store(0);
    health_monitoring_.memory_allocated_since_last_check.store(0);
    health_monitoring_.last_rate_calculation = now;
  }

  // Calculate trends
  metrics.hit_rate_trend = calculate_hit_rate_trend();
  metrics.latency_trend = calculate_latency_trend();

  // Set timestamp
  metrics.timestamp = now;

  // Determine health status
  metrics.status = determine_health_status(metrics);

  // Check alert conditions
  metrics.memory_pressure = metrics.memory_utilization > health_monitoring_.thresholds.memory_utilization_warning;
  metrics.performance_degradation = metrics.average_latency_ms > health_monitoring_.thresholds.latency_warning_ms;
  metrics.fragmentation_high = metrics.fragmentation_ratio > health_monitoring_.thresholds.fragmentation_warning;
  metrics.hit_rate_low = metrics.hit_rate < health_monitoring_.thresholds.hit_rate_warning;

  return metrics;
}

void MemoryPool::update_health_metrics() {
  auto now = std::chrono::steady_clock::now();

  // Check if it's time to update metrics
  if (now - health_monitoring_.last_metrics_update < health_monitoring_.METRICS_UPDATE_INTERVAL) {
    return;
  }

  // Try to acquire lock with timeout to avoid deadlock
  std::unique_lock<std::mutex> lock(health_mutex_, std::try_to_lock);
  if (!lock.owns_lock()) {
    return;  // Skip update if lock is not available
  }

  // Get current metrics
  HealthMetrics current_metrics = get_current_health_metrics();

  // Add to history
  add_health_metrics_to_history(current_metrics);

  // Update last update time
  health_monitoring_.last_metrics_update = now;

  // Check health status and generate alerts
  check_health_status();
}

void MemoryPool::check_health_status() {
  if (!health_monitoring_.alerts_enabled) {
    return;
  }

  if (health_monitoring_.metrics_history.empty()) {
    return;
  }

  const auto& latest_metrics = health_monitoring_.metrics_history.back();
  generate_alerts(latest_metrics);
  clear_resolved_alerts();
}

void MemoryPool::add_health_metrics_to_history(const HealthMetrics& metrics) {
  health_monitoring_.metrics_history.push_back(metrics);

  // Keep only the last MAX_HISTORY_SIZE entries
  if (health_monitoring_.metrics_history.size() > health_monitoring_.MAX_HISTORY_SIZE) {
    health_monitoring_.metrics_history.erase(
        health_monitoring_.metrics_history.begin(),
        health_monitoring_.metrics_history.begin() +
            (health_monitoring_.metrics_history.size() - health_monitoring_.MAX_HISTORY_SIZE));
  }
}

double MemoryPool::calculate_fragmentation_ratio() const {
  // Simple fragmentation calculation based on free buffer distribution
  size_t total_free_buffers = 0;
  size_t fragmented_blocks = 0;

  for (const auto& bucket : buckets_) {
    size_t free_count = bucket.free_count.load(std::memory_order_relaxed);
    total_free_buffers += free_count;

    // Count non-contiguous free blocks as fragmentation
    if (free_count > 0 && free_count < bucket.buffers.size()) {
      fragmented_blocks++;
    }
  }

  if (total_free_buffers == 0) {
    return 0.0;
  }

  return static_cast<double>(fragmented_blocks) / buckets_.size();
}

double MemoryPool::calculate_hit_rate_trend() const {
  if (health_monitoring_.metrics_history.size() < 2) {
    return 0.0;
  }

  const auto& recent = health_monitoring_.metrics_history.back();
  const auto& previous = health_monitoring_.metrics_history[health_monitoring_.metrics_history.size() - 2];

  return recent.hit_rate - previous.hit_rate;
}

double MemoryPool::calculate_latency_trend() const {
  if (health_monitoring_.metrics_history.size() < 2) {
    return 0.0;
  }

  const auto& recent = health_monitoring_.metrics_history.back();
  const auto& previous = health_monitoring_.metrics_history[health_monitoring_.metrics_history.size() - 2];

  return recent.average_latency_ms - previous.average_latency_ms;
}

double MemoryPool::calculate_memory_growth_rate() const {
  if (health_monitoring_.metrics_history.size() < 2) {
    return 0.0;
  }

  const auto& recent = health_monitoring_.metrics_history.back();
  const auto& previous = health_monitoring_.metrics_history[health_monitoring_.metrics_history.size() - 2];

  auto time_diff = std::chrono::duration<double>(recent.timestamp - previous.timestamp).count();
  if (time_diff <= 0) {
    return 0.0;
  }

  return (recent.memory_utilization - previous.memory_utilization) / time_diff;
}

MemoryPool::HealthMetrics::HealthStatus MemoryPool::determine_health_status(const HealthMetrics& metrics) const {
  const auto& thresholds = health_monitoring_.thresholds;

  // Check for critical conditions
  if (metrics.memory_utilization >= thresholds.memory_utilization_critical ||
      metrics.hit_rate <= thresholds.hit_rate_critical ||
      metrics.average_latency_ms >= thresholds.latency_critical_ms ||
      metrics.fragmentation_ratio >= thresholds.fragmentation_critical) {
    return HealthMetrics::HealthStatus::CRITICAL;
  }

  // Check for warning conditions
  if (metrics.memory_utilization >= thresholds.memory_utilization_warning ||
      metrics.hit_rate <= thresholds.hit_rate_warning || metrics.average_latency_ms >= thresholds.latency_warning_ms ||
      metrics.fragmentation_ratio >= thresholds.fragmentation_warning) {
    return HealthMetrics::HealthStatus::WARNING;
  }

  // Check for good performance
  if (metrics.hit_rate >= 0.5 && metrics.average_latency_ms <= 0.5 && metrics.memory_utilization <= 0.5 &&
      metrics.fragmentation_ratio <= 0.1) {
    return HealthMetrics::HealthStatus::EXCELLENT;
  }

  return HealthMetrics::HealthStatus::GOOD;
}

void MemoryPool::generate_alerts(const HealthMetrics& metrics) {
  const auto& thresholds = health_monitoring_.thresholds;

  // Clear existing alerts
  health_monitoring_.active_alerts.clear();

  // Memory pressure alerts
  if (metrics.memory_utilization >= thresholds.memory_utilization_critical) {
    health_monitoring_.active_alerts.push_back("CRITICAL: Memory utilization at " +
                                               std::to_string(metrics.memory_utilization * 100) + "%");
  } else if (metrics.memory_utilization >= thresholds.memory_utilization_warning) {
    health_monitoring_.active_alerts.push_back("WARNING: Memory utilization at " +
                                               std::to_string(metrics.memory_utilization * 100) + "%");
  }

  // Hit rate alerts
  if (metrics.hit_rate <= thresholds.hit_rate_critical) {
    health_monitoring_.active_alerts.push_back("CRITICAL: Hit rate at " + std::to_string(metrics.hit_rate * 100) + "%");
  } else if (metrics.hit_rate <= thresholds.hit_rate_warning) {
    health_monitoring_.active_alerts.push_back("WARNING: Hit rate at " + std::to_string(metrics.hit_rate * 100) + "%");
  }

  // Latency alerts
  if (metrics.average_latency_ms >= thresholds.latency_critical_ms) {
    health_monitoring_.active_alerts.push_back("CRITICAL: Average latency at " +
                                               std::to_string(metrics.average_latency_ms) + "ms");
  } else if (metrics.average_latency_ms >= thresholds.latency_warning_ms) {
    health_monitoring_.active_alerts.push_back("WARNING: Average latency at " +
                                               std::to_string(metrics.average_latency_ms) + "ms");
  }

  // Fragmentation alerts
  if (metrics.fragmentation_ratio >= thresholds.fragmentation_critical) {
    health_monitoring_.active_alerts.push_back("CRITICAL: Fragmentation ratio at " +
                                               std::to_string(metrics.fragmentation_ratio * 100) + "%");
  } else if (metrics.fragmentation_ratio >= thresholds.fragmentation_warning) {
    health_monitoring_.active_alerts.push_back("WARNING: Fragmentation ratio at " +
                                               std::to_string(metrics.fragmentation_ratio * 100) + "%");
  }
}

void MemoryPool::clear_resolved_alerts() {
  // This could be enhanced to track alert history and resolution
  // For now, we just clear alerts that are no longer active
}

// Public health monitoring interface
MemoryPool::HealthMetrics MemoryPool::get_health_status() const {
  std::lock_guard<std::mutex> lock(health_mutex_);
  return get_current_health_metrics();
}

std::vector<std::string> MemoryPool::get_active_alerts() const {
  std::lock_guard<std::mutex> lock(health_mutex_);
  return health_monitoring_.active_alerts;
}

void MemoryPool::set_alert_thresholds(const AlertThresholds& thresholds) {
  std::lock_guard<std::mutex> lock(health_mutex_);
  health_monitoring_.thresholds = thresholds;
}

void MemoryPool::enable_alerts(bool enable) {
  std::lock_guard<std::mutex> lock(health_mutex_);
  health_monitoring_.alerts_enabled = enable;
}

void MemoryPool::disable_alerts() {
  std::lock_guard<std::mutex> lock(health_mutex_);
  health_monitoring_.alerts_enabled = false;
}

std::vector<MemoryPool::HealthMetrics> MemoryPool::get_health_history(size_t max_entries) const {
  std::lock_guard<std::mutex> lock(health_mutex_);

  size_t start_index = 0;
  if (health_monitoring_.metrics_history.size() > max_entries) {
    start_index = health_monitoring_.metrics_history.size() - max_entries;
  }

  return std::vector<HealthMetrics>(health_monitoring_.metrics_history.begin() + start_index,
                                    health_monitoring_.metrics_history.end());
}

// ============================================================================
// PooledBuffer Implementation
// ============================================================================

PooledBuffer::PooledBuffer(size_t size) : size_(size), pool_(&GlobalMemoryPool::instance()) {
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

// Safe access methods for PooledBuffer
uint8_t& PooledBuffer::operator[](size_t index) {
  check_bounds(index);
  return buffer_.get()[index];
}

const uint8_t& PooledBuffer::operator[](size_t index) const {
  check_bounds(index);
  return buffer_.get()[index];
}

uint8_t* PooledBuffer::at(size_t offset) const {
  if (offset > size_) {
    throw std::out_of_range("Offset " + std::to_string(offset) + " exceeds buffer size " + std::to_string(size_));
  }
  return buffer_.get() + offset;
}

void PooledBuffer::check_bounds(size_t index) const {
  if (!valid()) {
    throw std::runtime_error("Accessing invalid buffer");
  }
  if (index >= size_) {
    throw std::out_of_range("Index " + std::to_string(index) + " exceeds buffer size " + std::to_string(size_));
  }
}

// ============================================================================
// Performance Optimization Methods
// ============================================================================

void MemoryPool::auto_tune() {
  if (!auto_tune_enabled_.load()) {
    return;
  }

  // Flush any pending local stats before auto-tuning
  flush_local_stats();

  std::lock_guard<std::mutex> usage_lock(usage_mutex_);

  // Analyze usage patterns and adjust pool configuration
  for (const auto& [size, count] : size_usage_count_) {
    if (count > common::constants::DEFAULT_MEMORY_POOL_SIZE) {  // Only tune for frequently used sizes
      double hit_rate = get_hit_rate_for_size(size);

      if (hit_rate < 0.5) {  // Low hit rate, increase pool size
        optimize_for_size(size, 0.8);
      }
    }
  }
}

MemoryPool::PoolStats MemoryPool::get_detailed_stats() const {
  // Flush any pending local stats before reading
  const_cast<MemoryPool*>(this)->flush_local_stats();

  std::lock_guard<std::mutex> stats_lock(stats_mutex_);
  PoolStats detailed_stats = stats_;

  // Calculate averages
  if (detailed_stats.total_allocations > 0) {
    detailed_stats.average_allocation_time_ms =
        static_cast<double>(detailed_stats.total_allocation_time) / detailed_stats.total_allocations;
  }

  if (detailed_stats.pool_hits + detailed_stats.pool_misses > 0) {
    detailed_stats.average_deallocation_time_ms = static_cast<double>(detailed_stats.total_deallocation_time) /
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
    size_t target_size = static_cast<size_t>(current_size * 1.5);  // 50% increase

    // Add more buffers to the pool with aligned allocation
    for (size_t i = current_size; i < target_size && i < max_pool_size_ / BUCKET_SIZES.size(); ++i) {
      bucket.buffers.emplace_back();
      BufferInfo& info = bucket.buffers.back();
      info.data = create_aligned_buffer(size);
      info.size = size;
      info.last_used = std::chrono::steady_clock::now();
      info.in_use = false;
      info.next_free = nullptr;

      // Add to free list for immediate availability
      add_to_free_list(bucket, &info);
    }
  }
}

double MemoryPool::get_hit_rate_for_size(size_t size) const {
  // Flush any pending local stats before reading
  const_cast<MemoryPool*>(this)->flush_local_stats();

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

// ============================================================================
// Free List Management Implementation
// ============================================================================

void MemoryPool::add_to_free_list(PoolBucket& bucket, MemoryPool::BufferInfo* buffer_info) {
  if (buffer_info == nullptr) return;

  // Add to head of free list (LIFO for better cache locality)
  buffer_info->next_free = bucket.free_list_head;
  bucket.free_list_head = buffer_info;
  bucket.free_count++;
}

MemoryPool::BufferInfo* MemoryPool::remove_from_free_list(PoolBucket& bucket) {
  if (bucket.free_list_head == nullptr) {
    return nullptr;
  }

  // Remove from head of free list
  BufferInfo* buffer_info = bucket.free_list_head;
  bucket.free_list_head = buffer_info->next_free;
  buffer_info->next_free = nullptr;
  bucket.free_count--;

  return buffer_info;
}

void MemoryPool::rebuild_free_list(PoolBucket& bucket) {
  // Clear existing free list
  bucket.free_list_head = nullptr;
  bucket.free_count = 0;

  // Rebuild free list from unused buffers
  for (auto& buffer_info : bucket.buffers) {
    if (!buffer_info.in_use) {
      add_to_free_list(bucket, &buffer_info);
    }
  }
}

}  // namespace common
}  // namespace unilink
