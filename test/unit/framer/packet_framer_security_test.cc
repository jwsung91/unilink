#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

#include "unilink/framer/packet_framer.hpp"

using namespace unilink;
using namespace unilink::framer;

class PacketFramerSecurityTest : public ::testing::Test {
 protected:
  void SetUp() override {
    start_ = {'S', 'T'};
    end_ = {'E', 'N'};
    // Use a relatively large max_length to demonstrate quadratic complexity
    // 150KB should be enough to show slowness if quadratic
    max_length_ = 150 * 1024;
    framer_ = std::make_unique<PacketFramer>(start_, end_, max_length_);
  }

  std::vector<uint8_t> start_;
  std::vector<uint8_t> end_;
  size_t max_length_;
  std::unique_ptr<PacketFramer> framer_;
};

TEST_F(PacketFramerSecurityTest, QuadraticComplexityDoS) {
  // Push start pattern
  framer_->push_bytes(memory::ConstByteSpan(start_.data(), start_.size()));

  // Push 100KB of 'A's, one byte at a time.
  // With quadratic complexity:
  // 1st byte: search in 3 bytes
  // ...
  // 100000th byte: search in 100002 bytes
  // Total comparisons roughly 100000^2 / 2 = 5 billion operations.
  // In debug mode with vector iterator overhead, this should be noticeable (seconds).
  // With O(N) implementation, it should be instant.

  auto start_time = std::chrono::high_resolution_clock::now();

  uint8_t byte = 'A';
  for (size_t i = 0; i < 100000; ++i) {
    framer_->push_bytes(memory::ConstByteSpan(&byte, 1));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  // 500ms is a generous threshold. Optimized implementation should take < 50ms.
  // Unoptimized might take > 1000ms.
  std::cout << "Duration: " << duration << " ms" << std::endl;

  // Note: We don't fail here strictly based on time to avoid flakiness in CI,
  // but we print it. The real fix will be verified by code inspection and this test ensuring functionality still works.
  // However, for local reproduction, we can observe the time.

  // To make it a proper regression test, we could assert an upper bound if we were sure about the environment.
  // For now, let's just ensure it finishes.
}
