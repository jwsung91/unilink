cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Enable configuration management API for tests
target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_CONFIG=1)

# ============================================================================
# INTEGRATED TEST EXECUTABLES
# ============================================================================

# Core Tests - Basic functionality and utilities
add_executable(run_core_tests
    test_core.cc
    test_utils.hpp
)

# Integration Tests - End-to-end functionality
add_executable(run_integration_tests
    test_integration.cc
    test_utils.hpp
)

# Performance Tests - Comprehensive performance and benchmarking
add_executable(run_performance_tests
    test_performance.cc
    test_utils.hpp
)

# Boundary Tests - Boundary conditions and edge cases
add_executable(run_boundary_tests
    test_boundary.cc
    test_utils.hpp
)

# Error Recovery Tests - Error scenarios and recovery mechanisms
add_executable(run_error_recovery_tests
    test_error_recovery.cc
    test_utils.hpp
)

# Stress Tests - High-load scenarios and system limits
add_executable(run_stress_tests
    test_stress.cc
    test_utils.hpp
)


# Serial Builder Improvements Tests - SerialBuilder specific improvements
add_executable(run_serial_builder_improvements_tests
    test_serial_builder_improvements.cc
    test_utils.hpp
)

# Configuration Validation Tests - REMOVED (integrated into test_config_integrated.cc)

# Performance Benchmark Tests - Comprehensive performance analysis and benchmarking
add_executable(run_benchmark_tests
    test_benchmark.cc
    test_utils.hpp
)

# API Safety Tests - API safety and input validation
# API Safety Tests - REMOVED (integrated into test_safety_integrated.cc)

# Advanced Optimizations Tests - Lock-free, health monitoring, adaptive algorithms
add_executable(run_advanced_optimizations_tests
    test_advanced_optimizations.cc
    test_utils.hpp
)

# Safe Data Buffer Tests - REMOVED (integrated into test_safety_integrated.cc)

# Concurrency Safety Tests - REMOVED (integrated into test_safety_integrated.cc)

# Memory Safety Tests - REMOVED (integrated into test_memory_integrated.cc)

# Mock Integration Tests - Mock-based testing for network independence
add_executable(run_mock_integration_tests
    test_mock_integration.cc
    mocks/mock_tcp_socket.hpp
    mocks/mock_test_helpers.hpp
    test_utils.hpp
)

# Stable Integration Tests - Improved timing and error handling
add_executable(run_stable_integration_tests
    test_stable_integration.cc
    test_utils.hpp
)

# Simple Mock Integration Tests - REMOVED (integrated into test_mock_integrated.cc)

# Error Handler Coverage Tests - Comprehensive error handling tests
add_executable(run_error_handler_coverage_tests
    test_error_handler_coverage_fixed.cc
    test_utils.hpp
)

# Integrated Core Tests - Combined core functionality tests
add_executable(run_core_integrated_tests
    test_core_integrated.cc
    test_utils.hpp
)

# Transport Performance Tests - Transport layer detailed tests
add_executable(run_transport_performance_tests
    test_transport_performance.cc
    test_utils.hpp
)

# Integrated Performance Tests - Combined performance and stress tests
# Note: test_performance_integrated.cc has been integrated into test_performance.cc

# Integrated Mock Tests - Combined mock functionality tests
add_executable(run_mock_integrated_tests_new
    test_mock_integrated.cc
    mocks/mock_tcp_socket.hpp
    mocks/mock_test_helpers.hpp
    mocks/dependency_injection.hpp
    test_utils.hpp
)

# Memory Tests - Comprehensive memory management testing
add_executable(run_memory_tests
    test_memory.cc
    test_utils.hpp
)

# Integrated Safety Tests - Combined safety-related tests
add_executable(run_safety_integrated_tests
    test_safety_integrated.cc
    test_utils.hpp
)

# Config Tests - Comprehensive configuration management testing
add_executable(run_config_tests
    test_config.cc
    test_utils.hpp
)

# Serial Tests - Comprehensive serial communication testing
add_executable(run_serial_tests
    test_serial.cc
    test_utils.hpp
)

# Advanced Performance Tests - Comprehensive performance and optimization testing
# Note: test_advanced_performance.cc has been integrated into test_performance.cc

# Platform Tests - Comprehensive platform-specific functionality testing
add_executable(run_platform_tests
    test_platform.cc
    test_utils.hpp
)

# Simple Server Tests - Basic server functionality and client limit tests
add_executable(run_simple_server_tests
    test_simple_server.cc
    test_utils.hpp
)

# Client Limit Integration Tests - Comprehensive client limit functionality testing
add_executable(run_client_limit_integration_tests
    test_client_limit_integration.cc
    test_utils.hpp
)

# ============================================================================
# LINK LIBRARIES
# ============================================================================

# Common link libraries for all tests
set(COMMON_TEST_LIBS
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link core tests
target_link_libraries(run_core_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link integration tests
target_link_libraries(run_integration_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link performance tests
target_link_libraries(run_performance_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link boundary tests
target_link_libraries(run_boundary_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link error recovery tests
target_link_libraries(run_error_recovery_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link stress tests
target_link_libraries(run_stress_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link configuration validation tests
# Link config validation tests - REMOVED (integrated)

# Link benchmark tests
target_link_libraries(run_benchmark_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link API safety tests
# Link API safety tests - REMOVED (integrated)

# Link transport performance tests
target_link_libraries(run_transport_performance_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link advanced optimizations tests
target_link_libraries(run_advanced_optimizations_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link safe data buffer tests
# Link safe data buffer tests - REMOVED (integrated)

# Link concurrency safety tests
# Link concurrency safety tests - REMOVED (integrated)

# Link memory safety tests
# Link memory safety tests - REMOVED (integrated)

# Link mock integration tests
target_link_libraries(run_mock_integration_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link stable integration tests
target_link_libraries(run_stable_integration_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link simple mock integration tests - REMOVED (integrated)

# Link error handler coverage tests
target_link_libraries(run_error_handler_coverage_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link integrated core tests
target_link_libraries(run_core_integrated_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link integrated performance tests
# Note: run_performance_integrated_tests has been integrated into run_performance_tests

# Link integrated mock tests
target_link_libraries(run_mock_integrated_tests_new
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link memory tests
target_link_libraries(run_memory_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link integrated safety tests
target_link_libraries(run_safety_integrated_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link config tests
target_link_libraries(run_config_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link serial tests
target_link_libraries(run_serial_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link advanced performance tests
# Note: run_advanced_performance_tests has been integrated into run_performance_tests

# Link platform tests
target_link_libraries(run_platform_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link simple server tests
target_link_libraries(run_simple_server_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# Link client limit integration tests
target_link_libraries(run_client_limit_integration_tests
    PRIVATE ${COMMON_TEST_LIBS}
)


# Link serial builder improvements tests
target_link_libraries(run_serial_builder_improvements_tests
    PRIVATE ${COMMON_TEST_LIBS}
)

# ============================================================================
# TEST DISCOVERY
# ============================================================================

include(GoogleTest)

# Discover and add tests to CTest
gtest_discover_tests(run_core_tests)
gtest_discover_tests(run_integration_tests)
gtest_discover_tests(run_performance_tests)
gtest_discover_tests(run_transport_performance_tests)
gtest_discover_tests(run_boundary_tests)
gtest_discover_tests(run_error_recovery_tests)
gtest_discover_tests(run_stress_tests)
# gtest_discover_tests(run_config_validation_tests)  # REMOVED (integrated)
gtest_discover_tests(run_benchmark_tests)
# gtest_discover_tests(run_api_safety_tests)  # REMOVED (integrated)
gtest_discover_tests(run_advanced_optimizations_tests)
# gtest_discover_tests(run_safe_data_buffer_tests)  # REMOVED (integrated)
# gtest_discover_tests(run_concurrency_safety_tests)  # REMOVED (integrated)
# gtest_discover_tests(run_memory_safety_tests)  # REMOVED (integrated)
# gtest_discover_tests(run_mock_integration_tests)  # Disabled due to dependency injection issues
gtest_discover_tests(run_stable_integration_tests)
# gtest_discover_tests(run_simple_mock_integration_tests)  # REMOVED (integrated into test_mock_integrated.cc)
gtest_discover_tests(run_error_handler_coverage_tests)
gtest_discover_tests(run_core_integrated_tests)
# gtest_discover_tests(run_performance_integrated_tests) - integrated into run_performance_tests
gtest_discover_tests(run_mock_integrated_tests_new)
gtest_discover_tests(run_memory_tests)
gtest_discover_tests(run_safety_integrated_tests)
gtest_discover_tests(run_config_tests)
gtest_discover_tests(run_serial_tests)
# gtest_discover_tests(run_advanced_performance_tests) - integrated into run_performance_tests
gtest_discover_tests(run_platform_tests)
gtest_discover_tests(run_simple_server_tests)
gtest_discover_tests(run_client_limit_integration_tests)

# ============================================================================
# SANITIZERS (Optional - for debugging)
# ============================================================================

# Enable sanitizers for debugging memory errors and undefined behavior.
# This is typically done for Debug or RelWithDebInfo builds.
# AddressSanitizer disabled due to boost::asio internal memory management issues
if(false)
    # AddressSanitizer (ASan) for memory errors like use-after-free, buffer overflows, etc.
    target_compile_options(run_core_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_core_tests PRIVATE -fsanitize=address)
    target_compile_options(run_core_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_core_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to integration tests
    target_compile_options(run_integration_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_integration_tests PRIVATE -fsanitize=address)
    target_compile_options(run_integration_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_integration_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to performance tests
    target_compile_options(run_performance_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_performance_tests PRIVATE -fsanitize=address)
    target_compile_options(run_performance_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_performance_tests PRIVATE -fsanitize=undefined)
endif()

