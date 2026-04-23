#include <gtest/gtest.h>
#include "unilink/memory/memory_tracker.hpp"
#include "unilink/memory/safe_data_buffer.hpp"
#include "unilink/diagnostics/logger.hpp"
#include <vector>
#include <thread>
#include <sstream>

using namespace unilink::memory;

class MemoryAdvancedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    MemoryTracker::instance().clear_tracking_data();
    MemoryTracker::instance().enable_tracking(true);
  }
  
  void TearDown() override {
    MemoryTracker::instance().clear_tracking_data();
  }
};

TEST_F(MemoryAdvancedTest, MemoryTrackerDetailedStats) {
  auto& tracker = MemoryTracker::instance();
  void* ptr1 = (void*)0x1234;
  void* ptr2 = (void*)0x5678;
  
  tracker.track_allocation(ptr1, 100, __FILE__, __LINE__, __FUNCTION__);
  tracker.track_allocation(ptr2, 200, __FILE__, __LINE__, __FUNCTION__);
  
  auto stats = tracker.stats();
  EXPECT_EQ(stats.total_allocations, 2);
  EXPECT_EQ(stats.current_bytes_allocated, 300);
  EXPECT_EQ(stats.peak_bytes_allocated, 300);
  
  tracker.track_deallocation(ptr1);
  stats = tracker.stats();
  EXPECT_EQ(stats.total_deallocations, 1);
  EXPECT_EQ(stats.current_bytes_allocated, 200);
}

TEST_F(MemoryAdvancedTest, MemoryTrackerControl) {
  auto& tracker = MemoryTracker::instance();
  tracker.disable_tracking();
  EXPECT_FALSE(tracker.tracking_enabled());
  
  void* ptr = (void*)0x9999;
  tracker.track_allocation(ptr, 500, __FILE__, __LINE__, __FUNCTION__);
  EXPECT_EQ(tracker.stats().total_allocations, 0);
  
  tracker.enable_tracking();
  tracker.track_allocation(ptr, 500, __FILE__, __LINE__, __FUNCTION__);
  EXPECT_EQ(tracker.stats().total_allocations, 1);
}

TEST_F(MemoryAdvancedTest, ScopedTracker) {
  auto& tracker = MemoryTracker::instance();
  void* ptr = (void*)0xAAAA;
  
  {
    ScopedMemoryTracker scoped(__FILE__, __LINE__, __FUNCTION__);
    scoped.track_allocation(ptr, 150);
    EXPECT_EQ(tracker.stats().current_bytes_allocated, 150);
    scoped.track_deallocation(ptr);
    EXPECT_EQ(tracker.stats().current_bytes_allocated, 0);
  }
}

TEST_F(MemoryAdvancedTest, ReportingMethods) {
  auto& tracker = MemoryTracker::instance();
  tracker.track_allocation((void*)0x1, 10, nullptr, 0, nullptr);
  
  // These print to stdout or log. Just ensuring they don't crash.
  EXPECT_NO_THROW(tracker.print_memory_report());
  EXPECT_NO_THROW(tracker.print_leak_report());
  EXPECT_NO_THROW(tracker.log_memory_report());
  EXPECT_NO_THROW(tracker.log_leak_report());
}

TEST_F(MemoryAdvancedTest, SafeDataBufferComprehensive) {
  std::string original = "Unilink Safe Buffer Test";
  SafeDataBuffer buffer(original);
  
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(buffer.size(), original.size());
  EXPECT_EQ(buffer.as_string(), original);
  
  // Access
  EXPECT_EQ(buffer[0], 'U');
  EXPECT_EQ(buffer.at(original.size() - 1), 't');
  EXPECT_THROW(buffer.at(original.size()), std::out_of_range);
  
  // Comparison
  SafeDataBuffer buffer2(original);
  EXPECT_EQ(buffer, buffer2);
  
  SafeDataBuffer buffer3(std::string("Different"));
  EXPECT_NE(buffer, buffer3);
  
  // Lifecycle
  buffer.clear();
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(buffer.size(), 0);
  
  buffer.reserve(100);
  buffer.resize(10);
  EXPECT_EQ(buffer.size(), 10);
}

TEST_F(MemoryAdvancedTest, MemoryTrackerLeakDetection) {
  auto& tracker = MemoryTracker::instance();
  tracker.track_allocation((void*)0xBBBB, 50, __FILE__, __LINE__, __FUNCTION__);
  
  auto leaks = tracker.leaked_allocations();
  ASSERT_EQ(leaks.size(), 1);
  EXPECT_EQ(leaks[0].size, 50);
  EXPECT_EQ(leaks[0].ptr, (void*)0xBBBB);
}
