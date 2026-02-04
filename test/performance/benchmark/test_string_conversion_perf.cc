#include <gtest/gtest.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "unilink/base/common.hpp"

using namespace unilink::base;

class StringConversionPerfTest : public ::testing::Test {
 protected:
  // Optimized implementation approach (simulated locally for baseline comparison)
  static std::pair<const uint8_t*, size_t> string_to_bytes_opt(const std::string& str) {
    return {reinterpret_cast<const uint8_t*>(str.data()), str.size()};
  }

  void RunBenchmark(const std::string& label, const std::string& data, size_t iterations) {
    // Measure Allocation Method (Current)
    auto start_alloc = std::chrono::high_resolution_clock::now();

    volatile size_t sink_alloc = 0;
    for (size_t i = 0; i < iterations; ++i) {
      auto vec = safe_convert::string_to_uint8(data);
      sink_alloc += vec.size();
      if (!vec.empty()) sink_alloc += vec[0];
    }

    auto end_alloc = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed_alloc = end_alloc - start_alloc;

    // Measure Zero-Copy Method (Optimized)
    auto start_opt = std::chrono::high_resolution_clock::now();

    volatile size_t sink_opt = 0;
    for (size_t i = 0; i < iterations; ++i) {
      auto pair = string_to_bytes_opt(data);
      sink_opt += pair.second;
      if (pair.second > 0) sink_opt += pair.first[0];
    }

    auto end_opt = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed_opt = end_opt - start_opt;

    // Output results
    std::cout << "\nBenchmark: " << label << " (" << iterations << " iterations)\n";
    std::cout << "  Original (Alloc): " << std::fixed << std::setprecision(2) << elapsed_alloc.count() << " us ("
              << (elapsed_alloc.count() / iterations) * 1000 << " ns/op)\n";
    std::cout << "  Optimized (View): " << std::fixed << std::setprecision(2) << elapsed_opt.count() << " us ("
              << (elapsed_opt.count() / iterations) * 1000 << " ns/op)\n";

    double speedup = elapsed_alloc.count() / elapsed_opt.count();
    std::cout << "  Speedup: " << speedup << "x\n";
  }
};

TEST_F(StringConversionPerfTest, CompareImplementations) {
  size_t iterations = 100000;
  std::string small_str = "Hello World";
  std::string medium_str(1024, 'A');
  std::string large_str(64 * 1024, 'B');  // 64KB

  RunBenchmark("Small String (11B)", small_str, iterations * 10);
  RunBenchmark("Medium String (1KB)", medium_str, iterations);
  RunBenchmark("Large String (64KB)", large_str, iterations / 10);
}
