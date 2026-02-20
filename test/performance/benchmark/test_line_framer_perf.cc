#include <gtest/gtest.h>

#include <chrono>
#include <random>
#include <string>
#include <vector>

#include "unilink/framer/line_framer.hpp"
#include "unilink/memory/safe_span.hpp"

using namespace unilink;
using namespace unilink::framer;

class LineFramerPerfTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  std::vector<uint8_t> generate_data(size_t total_size, size_t msg_size) {
    std::vector<uint8_t> data;
    data.reserve(total_size);
    std::string msg(msg_size, 'A');
    msg += '\n';
    while (data.size() < total_size) {
      data.insert(data.end(), msg.begin(), msg.end());
    }
    return data;
  }
};

TEST_F(LineFramerPerfTest, ProcessLargeDataChunks) {
  LineFramer framer;
  size_t msg_count = 0;
  framer.set_on_message([&](memory::ConstByteSpan) { msg_count++; });

  size_t total_size = 50 * 1024 * 1024;  // 50MB
  size_t msg_size = 100;
  auto data = generate_data(total_size, msg_size);

  auto start = std::chrono::high_resolution_clock::now();

  // Process in one go (simulating large read)
  framer.push_bytes(memory::ConstByteSpan(data.data(), data.size()));

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  double throughput = (data.size() * 1000000.0 / duration / 1024 / 1024);
  std::cout << "Processed " << data.size() << " bytes in " << duration << " us. " << "Throughput: " << throughput
            << " MB/s" << std::endl;
}

TEST_F(LineFramerPerfTest, ProcessSmallChunks) {
  LineFramer framer;
  size_t msg_count = 0;
  framer.set_on_message([&](memory::ConstByteSpan) { msg_count++; });

  size_t total_size = 10 * 1024 * 1024;  // 10MB
  size_t msg_size = 100;
  auto data = generate_data(total_size, msg_size);
  size_t chunk_size = 1024;  // 1KB chunks

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < data.size(); i += chunk_size) {
    size_t len = std::min(chunk_size, data.size() - i);
    framer.push_bytes(memory::ConstByteSpan(data.data() + i, len));
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  double throughput = (data.size() * 1000000.0 / duration / 1024 / 1024);
  std::cout << "Processed " << data.size() << " bytes in " << duration << " us (chunked). "
            << "Throughput: " << throughput << " MB/s" << std::endl;
}
