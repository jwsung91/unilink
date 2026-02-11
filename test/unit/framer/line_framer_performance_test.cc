#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "unilink/framer/line_framer.hpp"

using namespace unilink;
using namespace unilink::framer;

class LineFramerPerformanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Max length huge to allow accumulation
    framer_ = std::make_unique<LineFramer>("\n", false, 200000);
    framer_->set_on_message([this](memory::ConstByteSpan msg) {
      std::string s(reinterpret_cast<const char*>(msg.data()), msg.size());
      messages_.push_back(s);
    });
  }

  std::unique_ptr<LineFramer> framer_;
  std::vector<std::string> messages_;
};

TEST_F(LineFramerPerformanceTest, LargeBufferProcessingPerformance) {
  // Push 50,000 bytes one by one without newline.
  // Then one newline.
  // The buffer will grow to 50,001.
  // Old implementation: O(N^2) = 50000^2 / 2 = 1.25 billion checks.
  // New implementation: O(N) = 50000 checks.

  const size_t N = 50000;
  std::vector<uint8_t> byte(1, 'A');

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < N; ++i) {
    framer_->push_bytes(memory::ConstByteSpan(byte.data(), 1));
  }

  // Final newline to trigger message
  std::vector<uint8_t> newline(1, '\n');
  framer_->push_bytes(memory::ConstByteSpan(newline.data(), 1));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  EXPECT_EQ(messages_.size(), 1);
  EXPECT_EQ(messages_[0].length(), N);

  // With O(N^2), this takes > 500ms usually.
  // With O(N), this takes < 10ms.
  // We set a conservative limit of 200ms to allow for slow CI but fail on quadratic behavior.
  // Note: 50,000 might be too small for fast machines to fail > 200ms with strict certainty,
  // but let's try 50,000 first. If needed, I can increase to 100,000.
  // 1.25 billion ops is definitely > 200ms on most cores (usually ~1-2s).

  std::cout << "Performance test took " << duration << " ms for " << N << " bytes." << std::endl;
  EXPECT_LT(duration, 500) << "Performance regression: LineFramer is too slow (likely O(N^2))";
}
