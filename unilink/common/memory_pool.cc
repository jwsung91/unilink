/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "unilink/common/memory_pool.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace unilink {
namespace common {

// ============================================================================
// SelectiveMemoryPool Implementation
// ============================================================================

MemoryPool::MemoryPool(size_t initial_pool_size, size_t /* max_pool_size */) {
  // Initialize 4 fixed-size pools
  static constexpr std::array<size_t, 4> BUCKET_SIZES = {
      static_cast<size_t>(BufferSize::SMALL),   // 1KB
      static_cast<size_t>(BufferSize::MEDIUM),  // 4KB
      static_cast<size_t>(BufferSize::LARGE),   // 16KB
      static_cast<size_t>(BufferSize::XLARGE)   // 64KB
  };

  for (size_t i = 0; i < buckets_.size(); ++i) {
    buckets_[i].size_ = BUCKET_SIZES[i];
    buckets_[i].buffers_.reserve(initial_pool_size / BUCKET_SIZES.size());
  }
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire(size_t size) {
  validate_size(size);

  auto& bucket = get_bucket(size);
  return acquire_from_bucket(bucket);
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire(BufferSize buffer_size) {
  return acquire(static_cast<size_t>(buffer_size));
}

void MemoryPool::release(std::unique_ptr<uint8_t[]> buffer, size_t size) {
  if (!buffer) return;

  validate_size(size);

  auto& bucket = get_bucket(size);
  release_to_bucket(bucket, std::move(buffer));
}

MemoryPool::PoolStats MemoryPool::get_stats() const { return stats_; }

double MemoryPool::get_hit_rate() const {
  size_t total = stats_.total_allocations;
  if (total == 0) return 0.0;

  size_t hits = stats_.pool_hits;
  return static_cast<double>(hits) / static_cast<double>(total);
}

void MemoryPool::cleanup_old_buffers(std::chrono::milliseconds max_age) {
  // Simplified: cleanup functionality disabled
  (void)max_age;  // prevent unused parameter warning
}

std::pair<size_t, size_t> MemoryPool::get_memory_usage() const {
  // Simplified: return basic memory usage
  size_t current_usage = stats_.total_allocations * 4096;  // estimate average buffer size
  return std::make_pair(current_usage, current_usage);
}

void MemoryPool::resize_pool(size_t new_size) {
  // Simplified: resize functionality disabled
  (void)new_size;  // prevent unused parameter warning
}

void MemoryPool::auto_tune() {
  // Simplified: auto_tune functionality disabled
}

MemoryPool::HealthMetrics MemoryPool::get_health_metrics() const {
  HealthMetrics metrics;
  metrics.hit_rate = get_hit_rate();
  return metrics;
}

// ============================================================================
// Private helper functions
// ============================================================================

MemoryPool::PoolBucket& MemoryPool::get_bucket(size_t size) { return buckets_[get_bucket_index(size)]; }

size_t MemoryPool::get_bucket_index(size_t size) const {
  static constexpr std::array<size_t, 4> BUCKET_SIZES = {
      static_cast<size_t>(BufferSize::SMALL),   // 1KB
      static_cast<size_t>(BufferSize::MEDIUM),  // 4KB
      static_cast<size_t>(BufferSize::LARGE),   // 16KB
      static_cast<size_t>(BufferSize::XLARGE)   // 64KB
  };

  for (size_t i = 0; i < BUCKET_SIZES.size(); ++i) {
    if (size <= BUCKET_SIZES[i]) {
      return i;
    }
  }
  return BUCKET_SIZES.size() - 1;  // use largest size
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire_from_bucket(PoolBucket& bucket) {
  std::lock_guard<std::mutex> lock(bucket.mutex_);

  // Get from free list
  if (!bucket.free_indices_.empty()) {
    size_t index = bucket.free_indices_.front();
    bucket.free_indices_.pop();

    auto buffer = std::move(bucket.buffers_[index]);
    bucket.buffers_[index] = nullptr;

    stats_.pool_hits++;
    stats_.total_allocations++;

    return buffer;
  }

  // Create new buffer
  auto buffer = create_buffer(bucket.size_);
  if (buffer) {
    bucket.buffers_.push_back(nullptr);
    stats_.total_allocations++;
  }

  return buffer;
}

void MemoryPool::release_to_bucket(PoolBucket& bucket, std::unique_ptr<uint8_t[]> buffer) {
  std::lock_guard<std::mutex> lock(bucket.mutex_);

  // Add buffer back to pool and add index to free_indices
  if (bucket.buffers_.size() < bucket.buffers_.capacity()) {
    // Add within existing vector size
    size_t index = bucket.buffers_.size();
    bucket.buffers_.push_back(std::move(buffer));
    bucket.free_indices_.push(index);
  } else {
    // If pool is full, discard buffer (auto-release)
    // buffer is unique_ptr so it will be automatically deleted when out of scope
  }
}

std::unique_ptr<uint8_t[]> MemoryPool::create_buffer(size_t size) { return std::make_unique<uint8_t[]>(size); }

void MemoryPool::validate_size(size_t size) const {
  if (size == 0 || size > 64 * 1024 * 1024) {  // 64MB maximum
    throw std::invalid_argument("Invalid buffer size");
  }
}

// ============================================================================
// PoolBucket Move Constructor/Assignment Operator
// ============================================================================

MemoryPool::PoolBucket::PoolBucket(PoolBucket&& other) noexcept
    : buffers_(std::move(other.buffers_)), free_indices_(std::move(other.free_indices_)), mutex_(), size_(other.size_) {
  other.size_ = 0;
}

MemoryPool::PoolBucket& MemoryPool::PoolBucket::operator=(PoolBucket&& other) noexcept {
  if (this != &other) {
    buffers_ = std::move(other.buffers_);
    free_indices_ = std::move(other.free_indices_);
    size_ = other.size_;

    other.size_ = 0;
  }
  return *this;
}

// ============================================================================
// PooledBuffer Implementation
// ============================================================================

PooledBuffer::PooledBuffer(size_t size) : size_(size), pool_(&GlobalMemoryPool::instance()) {
  buffer_ = pool_->acquire(size);
}

PooledBuffer::PooledBuffer(MemoryPool::BufferSize buffer_size)
    : size_(static_cast<size_t>(buffer_size)), pool_(&GlobalMemoryPool::instance()) {
  buffer_ = pool_->acquire(size_);
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
    if (buffer_ && pool_) {
      try {
        pool_->release(std::move(buffer_), size_);
      } catch (...) {
        // Ignore exception and continue (noexcept function)
      }
    }

    buffer_ = std::move(other.buffer_);
    size_ = other.size_;
    pool_ = other.pool_;

    other.buffer_ = nullptr;
    other.size_ = 0;
    other.pool_ = nullptr;
  }
  return *this;
}

uint8_t* PooledBuffer::data() const { return buffer_.get(); }

size_t PooledBuffer::size() const { return size_; }

bool PooledBuffer::valid() const { return buffer_ != nullptr; }

uint8_t& PooledBuffer::operator[](size_t index) {
  if (!buffer_ || index >= size_) {
    throw std::out_of_range("Buffer index out of range");
  }
  return buffer_[index];
}

const uint8_t& PooledBuffer::operator[](size_t index) const {
  if (!buffer_ || index >= size_) {
    throw std::out_of_range("Buffer index out of range");
  }
  return buffer_[index];
}

uint8_t* PooledBuffer::at(size_t offset) const {
  if (!buffer_ || offset >= size_) {
    throw std::out_of_range("Buffer offset out of range");
  }
  return static_cast<uint8_t*>(buffer_.get()) + offset;
}

void PooledBuffer::check_bounds(size_t index) const {
  if (!buffer_ || index >= size_) {
    throw std::out_of_range("Buffer index out of range");
  }
}

}  // namespace common
}  // namespace unilink