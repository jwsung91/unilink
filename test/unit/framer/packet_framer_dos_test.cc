#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "unilink/framer/packet_framer.hpp"

using namespace unilink;
using namespace unilink::framer;

TEST(PacketFramerDoS, PerformanceCheck) {
  std::vector<uint8_t> start = {'S', 'T'};
  std::vector<uint8_t> end = {'E', 'N'};
  // Use a smaller size for the test to run quickly but still show quadratic behavior difference
  // O(N^2) with N=50000 -> ~1.25*10^9 ops. ~0.3s.
  // O(N) with N=50000 -> 5*10^4 ops. Instant.
  size_t max_len = 50000;
  PacketFramer framer(start, end, max_len);

  std::vector<uint8_t> payload;
  payload.insert(payload.end(), start.begin(), start.end());
  payload.resize(max_len - end.size(), 'A');
  payload.insert(payload.end(), end.begin(), end.end());

  auto start_time = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < payload.size(); ++i) {
    framer.push_bytes(memory::ConstByteSpan(&payload[i], 1));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  // If it's O(N^2), this will likely fail or be close to failing.
  // If it's O(N), it will be < 10ms.
  // We use 200ms as a safe threshold.
  EXPECT_LT(duration, 200) << "Packet processing took too long (" << duration
                           << "ms), indicating quadratic complexity.";
}
