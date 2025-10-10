#include "optimized_memory_pool.hpp"

#include <algorithm>
#include <chrono>

namespace unilink::common {

OptimizedMemoryPool::OptimizedMemoryPool(size_t small_initial, size_t small_max, size_t medium_initial,
                                         size_t medium_max, size_t large_initial, size_t large_max)
    : small_pool_(std::make_unique<MemoryPool>(small_initial, small_max)),
      medium_pool_(std::make_unique<MemoryPool>(medium_initial, medium_max)),
      large_pool_(std::make_unique<MemoryPool>(large_initial, large_max)) {}

std::unique_ptr<uint8_t[]> OptimizedMemoryPool::acquire(size_t size) {
  SizeCategory category = get_size_category(size);
  return get_pool(category).acquire(size);
}

void OptimizedMemoryPool::release(std::unique_ptr<uint8_t[]> buffer, size_t size) {
  if (!buffer) return;

  SizeCategory category = get_size_category(size);
  get_pool(category).release(std::move(buffer), size);
}

PoolStats OptimizedMemoryPool::get_stats() const {
  PoolStats combined_stats{};

  auto small_stats = small_pool_->get_stats();
  auto medium_stats = medium_pool_->get_stats();
  auto large_stats = large_pool_->get_stats();

  combined_stats.total_allocations =
      small_stats.total_allocations + medium_stats.total_allocations + large_stats.total_allocations;
  combined_stats.pool_hits = small_stats.pool_hits + medium_stats.pool_hits + large_stats.pool_hits;
  combined_stats.pool_misses = small_stats.pool_misses + medium_stats.pool_misses + large_stats.pool_misses;
  combined_stats.current_pool_size =
      small_stats.current_pool_size + medium_stats.current_pool_size + large_stats.current_pool_size;
  combined_stats.max_pool_size = small_stats.max_pool_size + medium_stats.max_pool_size + large_stats.max_pool_size;

  return combined_stats;
}

double OptimizedMemoryPool::get_hit_rate() const {
  auto stats = get_stats();
  if (stats.total_allocations == 0) return 0.0;
  return static_cast<double>(stats.pool_hits) / stats.total_allocations;
}

std::pair<size_t, size_t> OptimizedMemoryPool::get_memory_usage() const {
  auto small_usage = small_pool_->get_memory_usage();
  auto medium_usage = medium_pool_->get_memory_usage();
  auto large_usage = large_pool_->get_memory_usage();

  return {small_usage.first + medium_usage.first + large_usage.first,
          small_usage.second + medium_usage.second + large_usage.second};
}

PoolStats OptimizedMemoryPool::get_stats(SizeCategory category) const { return get_pool(category).get_stats(); }

double OptimizedMemoryPool::get_hit_rate(SizeCategory category) const { return get_pool(category).get_hit_rate(); }

void OptimizedMemoryPool::cleanup_old_buffers(std::chrono::milliseconds max_age) {
  small_pool_->cleanup_old_buffers(max_age);
  medium_pool_->cleanup_old_buffers(max_age);
  large_pool_->cleanup_old_buffers(max_age);
}

void OptimizedMemoryPool::resize_pool(size_t new_size) {
  small_pool_->resize_pool(new_size / 3);
  medium_pool_->resize_pool(new_size / 3);
  large_pool_->resize_pool(new_size / 3);
}

void OptimizedMemoryPool::auto_tune() {
  small_pool_->auto_tune();
  medium_pool_->auto_tune();
  large_pool_->auto_tune();
}

HealthMetrics OptimizedMemoryPool::get_health_metrics() const {
  HealthMetrics combined_metrics{};

  auto small_health = small_pool_->get_health_metrics();
  auto medium_health = medium_pool_->get_health_metrics();
  auto large_health = large_pool_->get_health_metrics();

  // Weighted average based on pool usage
  auto small_stats = small_pool_->get_stats();
  auto medium_stats = medium_pool_->get_stats();
  auto large_stats = large_pool_->get_stats();

  size_t total_allocations =
      small_stats.total_allocations + medium_stats.total_allocations + large_stats.total_allocations;

  if (total_allocations > 0) {
    double small_weight = static_cast<double>(small_stats.total_allocations) / total_allocations;
    double medium_weight = static_cast<double>(medium_stats.total_allocations) / total_allocations;
    double large_weight = static_cast<double>(large_stats.total_allocations) / total_allocations;

    combined_metrics.pool_utilization = small_health.pool_utilization * small_weight +
                                        medium_health.pool_utilization * medium_weight +
                                        large_health.pool_utilization * large_weight;

    combined_metrics.hit_rate = small_health.hit_rate * small_weight + medium_health.hit_rate * medium_weight +
                                large_health.hit_rate * large_weight;

    combined_metrics.memory_efficiency = small_health.memory_efficiency * small_weight +
                                         medium_health.memory_efficiency * medium_weight +
                                         large_health.memory_efficiency * large_weight;

    combined_metrics.performance_score = small_health.performance_score * small_weight +
                                         medium_health.performance_score * medium_weight +
                                         large_health.performance_score * large_weight;
  } else {
    // Default values when no allocations
    combined_metrics.pool_utilization = 0.0;
    combined_metrics.hit_rate = 0.0;
    combined_metrics.memory_efficiency = 0.0;
    combined_metrics.performance_score = 0.0;
  }

  return combined_metrics;
}

OptimizedMemoryPool::SizeCategory OptimizedMemoryPool::get_size_category(size_t size) const {
  if (size <= SMALL_THRESHOLD) {
    return SizeCategory::SMALL;
  } else if (size <= MEDIUM_THRESHOLD) {
    return SizeCategory::MEDIUM;
  } else {
    return SizeCategory::LARGE;
  }
}

MemoryPool& OptimizedMemoryPool::get_pool(SizeCategory category) {
  switch (category) {
    case SizeCategory::SMALL:
      return *small_pool_;
    case SizeCategory::MEDIUM:
      return *medium_pool_;
    case SizeCategory::LARGE:
      return *large_pool_;
    default:
      return *small_pool_;  // Default fallback
  }
}

const MemoryPool& OptimizedMemoryPool::get_pool(SizeCategory category) const {
  switch (category) {
    case SizeCategory::SMALL:
      return *small_pool_;
    case SizeCategory::MEDIUM:
      return *medium_pool_;
    case SizeCategory::LARGE:
      return *large_pool_;
    default:
      return *small_pool_;  // Default fallback
  }
}

// OptimizedPooledBuffer implementation
OptimizedPooledBuffer::OptimizedPooledBuffer(size_t size, OptimizedMemoryPool& pool) : size_(size), pool_(pool) {
  buffer_ = pool_.acquire(size);
}

OptimizedPooledBuffer::~OptimizedPooledBuffer() {
  if (buffer_) {
    pool_.release(std::move(buffer_), size_);
  }
}

OptimizedPooledBuffer::OptimizedPooledBuffer(OptimizedPooledBuffer&& other) noexcept
    : buffer_(std::move(other.buffer_)), size_(other.size_), pool_(other.pool_) {
  other.buffer_ = nullptr;
  other.size_ = 0;
}

OptimizedPooledBuffer& OptimizedPooledBuffer::operator=(OptimizedPooledBuffer&& other) noexcept {
  if (this != &other) {
    if (buffer_) {
      pool_.release(std::move(buffer_), size_);
    }

    buffer_ = std::move(other.buffer_);
    size_ = other.size_;
    pool_ = other.pool_;

    other.buffer_ = nullptr;
    other.size_ = 0;
  }
  return *this;
}

uint8_t* OptimizedPooledBuffer::data() const { return buffer_ ? buffer_.get() : nullptr; }

size_t OptimizedPooledBuffer::size() const { return size_; }

bool OptimizedPooledBuffer::valid() const { return buffer_ != nullptr; }

}  // namespace unilink::common
