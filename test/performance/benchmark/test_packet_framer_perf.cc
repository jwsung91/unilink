#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "unilink/framer/packet_framer.hpp"
#include "unilink/memory/safe_span.hpp"

using namespace unilink;
using namespace unilink::framer;

class PacketFramerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  std::vector<uint8_t> generate_data(size_t total_size, size_t payload_size) {
    std::vector<uint8_t> data;
    data.reserve(total_size);
    std::vector<uint8_t> start = {'S', 'T', 'A', 'R', 'T'};
    std::vector<uint8_t> end = {'E', 'N', 'D'};

    std::vector<uint8_t> packet;
    packet.insert(packet.end(), start.begin(), start.end());
    packet.insert(packet.end(), payload_size, 'X');
    packet.insert(packet.end(), end.begin(), end.end());

    while (data.size() < total_size) {
      data.insert(data.end(), packet.begin(), packet.end());
    }
    return data;
  }
};

TEST_F(PacketFramerPerfTest, ProcessLargeDataChunks) {
  std::vector<uint8_t> start_pattern = {'S', 'T', 'A', 'R', 'T'};
  std::vector<uint8_t> end_pattern = {'E', 'N', 'D'};
  PacketFramer framer(start_pattern, end_pattern, 1024);

  size_t msg_count = 0;
  framer.set_on_message([&](memory::ConstByteSpan) { msg_count++; });

  size_t total_size = 50 * 1024 * 1024;  // 50MB
  size_t payload_size = 100;
  auto data = generate_data(total_size, payload_size);

  auto start_time = std::chrono::high_resolution_clock::now();

  // Process in one go (simulating large read, common in network buffers)
  framer.push_bytes(memory::ConstByteSpan(data.data(), data.size()));

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

  double throughput = (static_cast<double>(data.size()) * 1000000.0 / static_cast<double>(duration) / 1024.0 / 1024.0);
  std::cout << "Processed " << data.size() << " bytes in " << duration << " us. " << "Throughput: " << throughput
            << " MB/s. " << "Messages: " << msg_count << std::endl;
}
