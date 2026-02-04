#include <gtest/gtest.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/memory/safe_span.hpp"

using namespace unilink::base;

class StringConversionPerfTest : public ::testing::Test {
 protected:
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
      auto span = safe_convert::string_to_uint8_span(data);
      sink_opt += span.size();
      if (span.size() > 0) sink_opt += span[0];
    }

    auto end_opt = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed_opt = end_opt - start_opt;

    // Output results
    std::cout << "\nBenchmark: " << label << " (" << iterations << " iterations)\n";
    std::cout << "  Original (Alloc): " << std::fixed << std::setprecision(2) << elapsed_alloc.count() << " us ("
              << (elapsed_alloc.count() / static_cast<double>(iterations)) * 1000.0 << " ns/op)\n";
    std::cout << "  Optimized (View): " << std::fixed << std::setprecision(2) << elapsed_opt.count() << " us ("
              << (elapsed_opt.count() / static_cast<double>(iterations)) * 1000.0 << " ns/op)\n";

    if (elapsed_opt.count() > 0) {
      double speedup = elapsed_alloc.count() / elapsed_opt.count();
      std::cout << "  Speedup: " << speedup << "x\n";
    }
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
