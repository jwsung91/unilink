/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <iostream>
#include "unilink/diagnostics/logger.hpp"

using namespace unilink::diagnostics;

class LoggerMacroBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        // Set log level to WARNING so DEBUG logs should be skipped
        Logger::instance().set_level(LogLevel::WARNING);
        // Disable console output to avoid I/O noise if it was somehow enabled
        Logger::instance().set_console_output(false);
    }
};

TEST_F(LoggerMacroBenchmark, DebugLogPerformance) {
    const int iterations = 100000;
    const std::string long_string(100, 'X'); // 100 chars to defeat SSO

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // This string concatenation should be avoided if the optimization works
        UNILINK_LOG_DEBUG("Benchmark", "Test",
            long_string + std::to_string(i));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "DEBUG Log Benchmark (Level=WARNING): " << iterations << " iterations took "
              << duration << " microseconds." << std::endl;

    std::cout << "Average time per call: " << (double)duration / iterations << " microseconds." << std::endl;
}

TEST_F(LoggerMacroBenchmark, InfoLogPerformance) {
    const int iterations = 100000;
    const std::string long_string(100, 'X'); // 100 chars to defeat SSO

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // This string concatenation should be avoided if the optimization works
        UNILINK_LOG_INFO("Benchmark", "Test",
            long_string + std::to_string(i));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "INFO Log Benchmark (Level=WARNING): " << iterations << " iterations took "
              << duration << " microseconds." << std::endl;

    std::cout << "Average time per call: " << (double)duration / iterations << " microseconds." << std::endl;
}
