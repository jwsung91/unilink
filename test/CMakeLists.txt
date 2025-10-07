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

# Performance Tests - Performance and benchmarking
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

# ============================================================================
# TEST DISCOVERY
# ============================================================================

include(GoogleTest)

# Discover and add tests to CTest
gtest_discover_tests(run_core_tests)
gtest_discover_tests(run_integration_tests)
gtest_discover_tests(run_performance_tests)
gtest_discover_tests(run_boundary_tests)
gtest_discover_tests(run_error_recovery_tests)
gtest_discover_tests(run_stress_tests)

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
