# Performance Tests - Benchmarks and profiling
# Each test file is built as a separate executable

option(BUILD_PERFORMANCE_TESTS "Build performance tests" ON)

if(BUILD_PERFORMANCE_TESTS)
  message(STATUS "Building performance tests")

  # Benchmark tests
  foreach(test_file test_performance.cc test_benchmark.cc test_transport_performance.cc test_platform.cc)
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(run_performance_${test_name} benchmark/${test_file})
    target_link_libraries(run_performance_${test_name}
      PRIVATE
        unilink
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
