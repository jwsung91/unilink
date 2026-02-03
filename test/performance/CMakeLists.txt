# Copyright 2025 Jinwoo Sung
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Performance Tests - Benchmarks and profiling
# Each test file is built as a separate executable

option(BUILD_PERFORMANCE_TESTS "Build performance tests" ON)

if(BUILD_PERFORMANCE_TESTS)
  message(STATUS "Building performance tests")

  # Benchmark tests
  foreach(test_file test_performance.cc test_benchmark.cc test_transport_performance.cc test_platform.cc test_logger_perf.cc test_tcp_callback_benchmark.cc test_console_logger_perf.cc)
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(run_performance_${test_name} benchmark/${test_file})
    target_link_libraries(run_performance_${test_name}
      PRIVATE
        unilink_shared
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
    )
    target_include_directories(run_performance_${test_name}
      PRIVATE
        ${CMAKE_SOURCE_DIR}/test/utils
        ${CMAKE_SOURCE_DIR}/test/fixtures
    )
    gtest_discover_tests(run_performance_${test_name}
      PROPERTIES
        LABELS "performance;benchmark;optional"
        TIMEOUT 180
    )
  endforeach()

  # Profiling tests - temporarily disabled due to link issues
  # add_executable(run_performance_test_advanced_optimizations profiling/test_advanced_optimizations.cc)
  # target_link_libraries(run_performance_test_advanced_optimizations
  #   PRIVATE
  #     unilink
  #     GTest::gtest
  #     GTest::gtest_main
  #     GTest::gmock
  # )
  # target_include_directories(run_performance_test_advanced_optimizations
  #   PRIVATE
  #     ${CMAKE_SOURCE_DIR}/test/utils
  #     ${CMAKE_SOURCE_DIR}/test/fixtures
  # )
  # gtest_discover_tests(run_performance_test_advanced_optimizations
  #   PROPERTIES
  #     LABELS "performance;profiling;optional"
  #     TIMEOUT 180
  # )
else()
  message(STATUS "Skipping performance tests (BUILD_PERFORMANCE_TESTS=OFF)")
endif()
