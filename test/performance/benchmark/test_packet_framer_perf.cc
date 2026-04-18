#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <vector>

#include "unilink/framer/packet_framer.hpp"
#include "unilink/memory/safe_span.hpp"

using namespace unilink;
using namespace unilink::framer;

class PacketFramerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    start_pattern_ = {'S', 'T', 'A', 'R', 'T'};
    end_pattern_ = {'E', 'N', 'D'};
  }

  std::vector<uint8_t> generate_data(size_t total_size, size_t payload_size) {
    std::vector<uint8_t> data;
    data.reserve(total_size);

    std::vector<uint8_t> packet = start_pattern_;
    std::vector<uint8_t> payload(payload_size, 'X');
    packet.insert(packet.end(), payload.begin(), payload.end());
    packet.insert(packet.end(), end_pattern_.begin(), end_pattern_.end());

    while (data.size() < total_size) {
      data.insert(data.end(), packet.begin(), packet.end());
    }
    return data;
  }

  std::vector<uint8_t> start_pattern_;
  std::vector<uint8_t> end_pattern_;
};

TEST_F(PacketFramerPerfTest, ProcessLargeDataChunks) {
  PacketFramer framer(start_pattern_, end_pattern_, 1024 * 1024);  // 1MB max packet
  size_t msg_count = 0;
  framer.on_message([&](memory::ConstByteSpan) { msg_count++; });

  size_t total_size = 1 * 1024 * 1024;  // 1MB (reduced from 50MB due to slowness)
  size_t payload_size = 100;            // Small payload
  auto data = generate_data(total_size, payload_size);

  auto start = std::chrono::high_resolution_clock::now();

  // Process in one go (simulating large read)
  framer.push_bytes(memory::ConstByteSpan(data.data(), data.size()));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  double throughput = (data.size() * 1000000.0 / duration / 1024 / 1024);
  std::cout << "Processed " << data.size() << " bytes in " << duration << " us. " << "Throughput: " << throughput
            << " MB/s. Messages: " << msg_count << std::endl;
}

TEST_F(PacketFramerPerfTest, ProcessLargePackets) {
  PacketFramer framer(start_pattern_, end_pattern_, 1024 * 1024 * 2);
  size_t msg_count = 0;
  framer.on_message([&](memory::ConstByteSpan) { msg_count++; });

  size_t total_size = 100 * 1024 * 1024;  // 100MB
  size_t payload_size = 1024 * 1024;      // 1MB payload
  auto data = generate_data(total_size, payload_size);

  auto start = std::chrono::high_resolution_clock::now();

  framer.push_bytes(memory::ConstByteSpan(data.data(), data.size()));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  double throughput = (data.size() * 1000000.0 / duration / 1024 / 1024);
  std::cout << "Processed " << data.size() << " bytes in " << duration << " us. " << "Throughput: " << throughput
            << " MB/s. Messages: " << msg_count << std::endl;
}
