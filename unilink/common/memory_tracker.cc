#include "unilink/common/memory_tracker.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "unilink/common/logger.hpp"

namespace unilink {
namespace common {

MemoryTracker& MemoryTracker::instance() {
  static MemoryTracker instance;
  return instance;
}

void MemoryTracker::track_allocation(void* ptr, size_t size, const char* file, int line, const char* function) {
  if (!tracking_enabled_.load()) {
    return;
  }

  std::lock_guard<std::mutex> lock(allocations_mutex_);

  AllocationInfo info;
  info.ptr = ptr;
  info.size = size;
  info.file = file ? file : "unknown";
  info.line = line;
  info.function = function ? function : "unknown";
  info.timestamp = std::chrono::steady_clock::now();

  allocations_[ptr] = info;

  // Update statistics
  stats_.total_allocations++;
  stats_.current_allocations++;
  stats_.total_bytes_allocated += size;
  stats_.current_bytes_allocated += size;

  // Update peak values
  if (stats_.current_allocations > stats_.peak_allocations) {
    stats_.peak_allocations = stats_.current_allocations;
  }
  if (stats_.current_bytes_allocated > stats_.peak_bytes_allocated) {
    stats_.peak_bytes_allocated = stats_.current_bytes_allocated;
  }
}

void MemoryTracker::track_deallocation(void* ptr) {
  if (!tracking_enabled_.load()) {
    return;
  }

  std::lock_guard<std::mutex> lock(allocations_mutex_);

  auto it = allocations_.find(ptr);
  if (it != allocations_.end()) {
    size_t size = it->second.size;
    allocations_.erase(it);

    // Update statistics
    stats_.total_deallocations++;
    stats_.current_allocations--;
    stats_.total_bytes_deallocated += size;
    stats_.current_bytes_allocated -= size;
  }
}

MemoryTracker::MemoryStats MemoryTracker::get_stats() const { return stats_; }

std::vector<MemoryTracker::AllocationInfo> MemoryTracker::get_current_allocations() const {
  std::lock_guard<std::mutex> lock(allocations_mutex_);

  std::vector<AllocationInfo> result;
  result.reserve(allocations_.size());

  for (const auto& pair : allocations_) {
    result.push_back(pair.second);
  }

  return result;
}

std::vector<MemoryTracker::AllocationInfo> MemoryTracker::get_leaked_allocations() const {
  return get_current_allocations();  // Current allocations are potential leaks
}

void MemoryTracker::enable_tracking(bool enable) { tracking_enabled_.store(enable); }

void MemoryTracker::disable_tracking() { tracking_enabled_.store(false); }

bool MemoryTracker::is_tracking_enabled() const { return tracking_enabled_.load(); }

void MemoryTracker::clear_tracking_data() {
  std::lock_guard<std::mutex> lock(allocations_mutex_);
  allocations_.clear();

  // Reset statistics
  stats_.total_allocations = 0;
  stats_.total_deallocations = 0;
  stats_.current_allocations = 0;
  stats_.peak_allocations = 0;
  stats_.total_bytes_allocated = 0;
  stats_.total_bytes_deallocated = 0;
  stats_.current_bytes_allocated = 0;
  stats_.peak_bytes_allocated = 0;
}

void MemoryTracker::print_memory_report() const {
  auto stats = get_stats();
  auto current_allocations = get_current_allocations();

  std::cout << "\n=== Memory Tracker Report ===" << std::endl;
  std::cout << "Total allocations: " << stats.total_allocations << std::endl;
  std::cout << "Total deallocations: " << stats.total_deallocations << std::endl;
  std::cout << "Current allocations: " << stats.current_allocations << std::endl;
  std::cout << "Peak allocations: " << stats.peak_allocations << std::endl;
  std::cout << "Total bytes allocated: " << stats.total_bytes_allocated << std::endl;
  std::cout << "Total bytes deallocated: " << stats.total_bytes_deallocated << std::endl;
  std::cout << "Current bytes allocated: " << stats.current_bytes_allocated << std::endl;
  std::cout << "Peak bytes allocated: " << stats.peak_bytes_allocated << std::endl;
  std::cout << "Current active allocations: " << current_allocations.size() << std::endl;

  if (!current_allocations.empty()) {
    std::cout << "\n=== Current Allocations ===" << std::endl;
    for (const auto& alloc : current_allocations) {
      std::cout << "Ptr: " << alloc.ptr << ", Size: " << alloc.size << ", File: " << alloc.file << ":" << alloc.line
                << ", Function: " << alloc.function << std::endl;
    }
  }
}

void MemoryTracker::print_leak_report() const {
  auto leaked_allocations = get_leaked_allocations();

  if (leaked_allocations.empty()) {
    std::cout << "\n=== No Memory Leaks Detected ===" << std::endl;
    return;
  }

  std::cout << "\n=== Memory Leak Report ===" << std::endl;
  std::cout << "Found " << leaked_allocations.size() << " potential memory leaks:" << std::endl;

  size_t total_leaked_bytes = 0;
  for (const auto& alloc : leaked_allocations) {
    std::cout << "Leaked: " << alloc.size << " bytes at " << alloc.ptr << " allocated in " << alloc.file << ":"
              << alloc.line << " (" << alloc.function << ")" << std::endl;
    total_leaked_bytes += alloc.size;
  }

  std::cout << "Total leaked bytes: " << total_leaked_bytes << std::endl;
}

void MemoryTracker::log_memory_report() const {
  auto stats = get_stats();
  auto current_allocations = get_current_allocations();

  std::ostringstream oss;
  oss << "\n=== Memory Tracker Report ===\n";
  oss << "Total allocations: " << stats.total_allocations << "\n";
  oss << "Total deallocations: " << stats.total_deallocations << "\n";
  oss << "Current allocations: " << stats.current_allocations << "\n";
  oss << "Peak allocations: " << stats.peak_allocations << "\n";
  oss << "Total bytes allocated: " << stats.total_bytes_allocated << "\n";
  oss << "Total bytes deallocated: " << stats.total_bytes_deallocated << "\n";
  oss << "Current bytes allocated: " << stats.current_bytes_allocated << "\n";
  oss << "Peak bytes allocated: " << stats.peak_bytes_allocated << "\n";
  oss << "Current active allocations: " << current_allocations.size();

  UNILINK_LOG_INFO("memory_tracker", "report", oss.str());

  if (!current_allocations.empty()) {
    std::ostringstream alloc_oss;
    alloc_oss << "\n=== Current Allocations ===\n";
    for (const auto& alloc : current_allocations) {
      alloc_oss << "Ptr: " << alloc.ptr << ", Size: " << alloc.size << ", File: " << alloc.file << ":" << alloc.line
                << ", Function: " << alloc.function << "\n";
    }
    UNILINK_LOG_INFO("memory_tracker", "allocations", alloc_oss.str());
  }
}

void MemoryTracker::log_leak_report() const {
  auto leaked_allocations = get_leaked_allocations();

  if (leaked_allocations.empty()) {
    UNILINK_LOG_INFO("memory_tracker", "leak_check", "No Memory Leaks Detected");
    return;
  }

  std::ostringstream oss;
  oss << "\n=== Memory Leak Report ===\n";
  oss << "Found " << leaked_allocations.size() << " potential memory leaks:\n";

  size_t total_leaked_bytes = 0;
  for (const auto& alloc : leaked_allocations) {
    oss << "Leaked: " << alloc.size << " bytes at " << alloc.ptr << " allocated in " << alloc.file << ":" << alloc.line
        << " (" << alloc.function << ")\n";
    total_leaked_bytes += alloc.size;
  }

  oss << "Total leaked bytes: " << total_leaked_bytes;
  UNILINK_LOG_ERROR("memory_tracker", "leak_check", oss.str());
}

// ScopedMemoryTracker implementation
ScopedMemoryTracker::ScopedMemoryTracker(const char* file, int line, const char* function)
    : file_(file), line_(line), function_(function) {}

ScopedMemoryTracker::~ScopedMemoryTracker() {
  // Destructor can be used for cleanup if needed
}

void ScopedMemoryTracker::track_allocation(void* ptr, size_t size) {
  MemoryTracker::instance().track_allocation(ptr, size, file_, line_, function_);
}

void ScopedMemoryTracker::track_deallocation(void* ptr) { MemoryTracker::instance().track_deallocation(ptr); }

}  // namespace common
}  // namespace unilink
