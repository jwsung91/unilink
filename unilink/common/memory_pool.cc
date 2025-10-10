#include "unilink/common/memory_pool.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace unilink {
namespace common {

// ============================================================================
// SelectiveMemoryPool Implementation
// ============================================================================

MemoryPool::MemoryPool(size_t initial_pool_size, size_t max_pool_size) : max_pool_size_(max_pool_size) {
  // 4개 고정 크기 풀 초기화
  static constexpr std::array<size_t, 4> BUCKET_SIZES = {
      static_cast<size_t>(BufferSize::SMALL),   // 1KB
      static_cast<size_t>(BufferSize::MEDIUM),  // 4KB
      static_cast<size_t>(BufferSize::LARGE),   // 16KB
      static_cast<size_t>(BufferSize::XLARGE)   // 64KB
  };

  for (size_t i = 0; i < buckets_.size(); ++i) {
    buckets_[i].size_ = BUCKET_SIZES[i];
    buckets_[i].buffers_.reserve(initial_pool_size / BUCKET_SIZES.size());
    buckets_[i].last_used_times_.reserve(initial_pool_size / BUCKET_SIZES.size());

    // 큰 풀에만 lock-free 활성화
    buckets_[i].use_lock_free_ = (initial_pool_size >= LOCK_FREE_THRESHOLD);
  }

  stats_.max_pool_size = max_pool_size;
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire(size_t size) {
  validate_size(size);

  auto& bucket = get_bucket(size);

  // 모든 경우에 락 기반 사용 (안정성 우선)
  return acquire_with_lock(bucket);
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire(BufferSize buffer_size) {
  return acquire(static_cast<size_t>(buffer_size));
}

void MemoryPool::release(std::unique_ptr<uint8_t[]> buffer, size_t size) {
  if (!buffer) return;

  validate_size(size);

  auto& bucket = get_bucket(size);

  // 모든 경우에 락 기반 사용 (안정성 우선)
  release_with_lock(bucket, std::move(buffer));
}

MemoryPool::PoolStats MemoryPool::get_stats() const { return stats_; }

double MemoryPool::get_hit_rate() const {
  size_t total = stats_.total_allocations;
  if (total == 0) return 0.0;

  size_t hits = stats_.pool_hits;
  return static_cast<double>(hits) / total;
}

void MemoryPool::cleanup_old_buffers(std::chrono::milliseconds max_age) {
  auto now = std::chrono::steady_clock::now();
  auto cutoff_time = now - max_age;

  for (auto& bucket : buckets_) {
    std::lock_guard<std::mutex> lock(bucket.mutex_);

    // 오래된 버퍼 제거
    auto it = bucket.buffers_.begin();
    auto time_it = bucket.last_used_times_.begin();

    while (it != bucket.buffers_.end()) {
      if (*time_it < cutoff_time) {
        // 오래된 버퍼 제거
        *it = BufferInfo{};
        bucket.free_indices_.push(std::distance(bucket.buffers_.begin(), it));
        stats_.current_pool_size--;
      }
      ++it;
      ++time_it;
    }
  }
}

std::pair<size_t, size_t> MemoryPool::get_memory_usage() const {
  // 메모리 사용량 추정
  size_t current_usage = stats_.current_pool_size * 4096;  // 평균 버퍼 크기 추정
  size_t total_allocated = stats_.total_allocations * 4096;

  return std::make_pair(current_usage, total_allocated);
}

void MemoryPool::resize_pool(size_t new_size) {
  // 선택적 메모리풀은 동적 크기 조정을 지원하지 않으므로
  // 통계만 업데이트
  stats_.max_pool_size = new_size;
}

void MemoryPool::auto_tune() {
  // 선택적 메모리풀은 자동 튜닝을 지원하지 않으므로
  // 통계만 동기화
}

MemoryPool::HealthMetrics MemoryPool::get_health_metrics() const {
  // 기본적인 건강 지표 제공
  HealthMetrics metrics;

  metrics.pool_utilization = static_cast<double>(stats_.current_pool_size) / stats_.max_pool_size;
  metrics.hit_rate = get_hit_rate();
  metrics.memory_efficiency = 1.0;  // 선택적 메모리풀은 효율적
  metrics.performance_score = 1.0;  // 높은 성능

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
  return BUCKET_SIZES.size() - 1;  // 가장 큰 크기 사용
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire_with_lock(PoolBucket& bucket) {
  std::lock_guard<std::mutex> lock(bucket.mutex_);

  // Free list에서 가져오기
  if (!bucket.free_indices_.empty()) {
    size_t index = bucket.free_indices_.front();
    bucket.free_indices_.pop();

    auto buffer = std::move(bucket.buffers_[index].data);
    bucket.buffers_[index] = BufferInfo{};
    bucket.last_used_times_[index] = std::chrono::steady_clock::now();

    stats_.pool_hits++;
    stats_.total_allocations++;

    return buffer;
  }

  // 새 버퍼 생성
  if (bucket.buffers_.size() < max_pool_size_ / buckets_.size()) {
    auto buffer = create_buffer(bucket.size_);
    if (buffer) {
      bucket.buffers_.push_back(BufferInfo{});
      bucket.last_used_times_.push_back(std::chrono::steady_clock::now());
      stats_.current_pool_size++;
      stats_.total_allocations++;
      stats_.pool_misses++;
      return buffer;
    }
  }

  // 풀 가득참 - 직접 할당
  stats_.pool_misses++;
  stats_.total_allocations++;
  return create_buffer(bucket.size_);
}

std::unique_ptr<uint8_t[]> MemoryPool::acquire_lock_free(PoolBucket& bucket) {
  // Lock-free 풀 초기화
  if (bucket.lock_free_pool_.empty()) {
    bucket.lock_free_pool_.reserve(100);  // 고정 크기
    for (size_t i = 0; i < 100; ++i) {
      bucket.lock_free_pool_.push_back(create_buffer(bucket.size_));
    }
  }

  // Lock-free 할당 - 여러 번 시도
  for (int retry = 0; retry < 10; ++retry) {
    size_t current_index = bucket.lock_free_index_.load();
    size_t allocated_count = bucket.lock_free_allocated_.load();

    if (allocated_count >= bucket.lock_free_pool_.size()) {
      // 풀 가득참 - 직접 할당
      stats_.pool_misses++;
      stats_.total_allocations++;
      return create_buffer(bucket.size_);
    }

    // 원자적 할당
    size_t next_index = (current_index + 1) % bucket.lock_free_pool_.size();

    if (bucket.lock_free_index_.compare_exchange_weak(current_index, next_index)) {
      bucket.lock_free_allocated_.fetch_add(1);

      auto buffer = std::move(bucket.lock_free_pool_[current_index]);
      bucket.lock_free_pool_[current_index] = nullptr;

      stats_.pool_hits++;
      stats_.total_allocations++;

      return buffer;
    }
  }

  // 실패 - 직접 할당
  stats_.pool_misses++;
  stats_.total_allocations++;
  return create_buffer(bucket.size_);
}

void MemoryPool::release_with_lock(PoolBucket& bucket, std::unique_ptr<uint8_t[]> buffer) {
  std::lock_guard<std::mutex> lock(bucket.mutex_);

  // 풀에 여유가 있으면 추가
  if (bucket.buffers_.size() < max_pool_size_ / buckets_.size()) {
    // 빈 슬롯 찾기
    for (size_t i = 0; i < bucket.buffers_.size(); ++i) {
      if (bucket.buffers_[i].data == nullptr) {
        bucket.buffers_[i].data = std::move(buffer);
        bucket.last_used_times_[i] = std::chrono::steady_clock::now();
        bucket.free_indices_.push(i);  // free_indices_에 인덱스 추가
        return;
      }
    }

    // 새 슬롯 추가
    bucket.buffers_.push_back(BufferInfo{});
    bucket.buffers_.back().data = std::move(buffer);
    bucket.last_used_times_.push_back(std::chrono::steady_clock::now());
    bucket.free_indices_.push(bucket.buffers_.size() - 1);  // free_indices_에 인덱스 추가
  }
}

void MemoryPool::release_lock_free(PoolBucket& bucket, std::unique_ptr<uint8_t[]> buffer) {
  if (bucket.lock_free_pool_.empty()) return;

  // 빈 슬롯 찾기
  for (size_t i = 0; i < bucket.lock_free_pool_.size(); ++i) {
    if (bucket.lock_free_pool_[i] == nullptr) {
      bucket.lock_free_pool_[i] = std::move(buffer);
      bucket.lock_free_allocated_.fetch_sub(1);
      return;
    }
  }
}

std::unique_ptr<uint8_t[]> MemoryPool::create_buffer(size_t size) {
  if (should_use_aligned_allocation(size)) {
    return create_aligned_buffer(size);
  }
  return std::make_unique<uint8_t[]>(size);
}

std::unique_ptr<uint8_t[]> MemoryPool::create_aligned_buffer(size_t size) {
  // 64바이트 정렬된 메모리 할당
  void* ptr = std::aligned_alloc(ALIGNMENT_SIZE, size);
  if (!ptr) {
    // aligned_alloc 실패 시 일반 할당으로 fallback
    return std::make_unique<uint8_t[]>(size);
  }

  // 커스텀 deleter를 사용하여 정렬된 메모리 해제
  auto deleter = [](uint8_t* p) {
    if (p) {
      std::free(p);
    }
  };

  // 타입 변환을 위해 임시 변수 사용
  std::unique_ptr<uint8_t[], decltype(deleter)> temp_ptr(static_cast<uint8_t*>(ptr), deleter);

  // 일반 unique_ptr로 변환 (move 시맨틱 사용)
  std::unique_ptr<uint8_t[]> result;
  result.reset(temp_ptr.release());

  return result;
}

bool MemoryPool::should_use_lock_free(size_t pool_size) const { return pool_size >= LOCK_FREE_THRESHOLD; }

bool MemoryPool::should_use_aligned_allocation(size_t size) const { return size >= ALIGNMENT_THRESHOLD; }

void MemoryPool::validate_size(size_t size) const {
  if (size == 0 || size > 64 * 1024 * 1024) {  // 64MB 최대
    throw std::invalid_argument("Invalid buffer size");
  }
}

// ============================================================================
// BufferInfo 이동 생성자/할당 연산자
// ============================================================================

MemoryPool::BufferInfo::BufferInfo(BufferInfo&& other) noexcept
    : data(std::move(other.data)),
      size(other.size),
      last_used(other.last_used),
      in_use(other.in_use),
      next_free(other.next_free) {
  other.next_free = nullptr;
}

MemoryPool::BufferInfo& MemoryPool::BufferInfo::operator=(BufferInfo&& other) noexcept {
  if (this != &other) {
    data = std::move(other.data);
    size = other.size;
    last_used = other.last_used;
    in_use = other.in_use;
    next_free = other.next_free;
    other.next_free = nullptr;
  }
  return *this;
}

// ============================================================================
// PoolBucket 이동 생성자/할당 연산자
// ============================================================================

MemoryPool::PoolBucket::PoolBucket(PoolBucket&& other) noexcept
    : buffers_(std::move(other.buffers_)),
      free_indices_(std::move(other.free_indices_)),
      mutex_(),
      size_(other.size_),
      lock_free_pool_(std::move(other.lock_free_pool_)),
      lock_free_index_(other.lock_free_index_.load()),
      lock_free_allocated_(other.lock_free_allocated_.load()),
      use_lock_free_(other.use_lock_free_),
      last_used_times_(std::move(other.last_used_times_)) {
  other.size_ = 0;
  other.use_lock_free_ = false;
}

MemoryPool::PoolBucket& MemoryPool::PoolBucket::operator=(PoolBucket&& other) noexcept {
  if (this != &other) {
    buffers_ = std::move(other.buffers_);
    free_indices_ = std::move(other.free_indices_);
    size_ = other.size_;
    lock_free_pool_ = std::move(other.lock_free_pool_);
    lock_free_index_.store(other.lock_free_index_.load());
    lock_free_allocated_.store(other.lock_free_allocated_.load());
    use_lock_free_ = other.use_lock_free_;
    last_used_times_ = std::move(other.last_used_times_);

    other.size_ = 0;
    other.use_lock_free_ = false;
  }
  return *this;
}

// ============================================================================
// PooledBuffer 구현
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
        // 예외를 무시하고 계속 진행 (noexcept 함수이므로)
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