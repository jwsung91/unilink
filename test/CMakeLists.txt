cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Enable configuration management API for tests
target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_CONFIG=1)

# ============================================================================
# NEW ORGANIZED TEST STRUCTURE
# ============================================================================

# Include GoogleTest for test discovery
include(GoogleTest)

# Add subdirectories with organized tests
message(STATUS "Building Unit Tests")
add_subdirectory(unit)

message(STATUS "Building Integration Tests")
add_subdirectory(integration)

message(STATUS "Building E2E Tests")
add_subdirectory(e2e)

message(STATUS "Building Performance Tests (optional)")
add_subdirectory(performance)

# ============================================================================
# LEGACY TESTS - To be migrated or removed
# ============================================================================

# The following tests are kept temporarily for backward compatibility.
# They will be removed once all tests are verified in the new structure.

# Check if there are any remaining test files in the root test/ directory
file(GLOB REMAINING_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cc")
if(REMAINING_TESTS)
  message(WARNING "Found ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cc files that haven't been migrated:")
  foreach(test_file ${REMAINING_TESTS})
    message(WARNING "  - ${test_file}")
  endforeach()
  
  # Keep old structure temporarily
  # Architecture Tests
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_architecture.cc")
    add_executable(run_architecture_tests
      test_architecture.cc
      test_utils.hpp
    )
    target_link_libraries(run_architecture_tests
      PRIVATE unilink GTest::gtest_main GTest::gmock
    )
    gtest_discover_tests(run_architecture_tests
      PROPERTIES LABELS "legacy;e2e"
    )
  endif()
  
  # Builder Tests
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_builder.cc")
    add_executable(run_builder_tests
      test_builder.cc
      test_utils.hpp
    )
    target_link_libraries(run_builder_tests
      PRIVATE unilink GTest::gtest_main GTest::gmock
    )
    gtest_discover_tests(run_builder_tests
      PROPERTIES LABELS "legacy;unit"
    )
  endif()
  
  # Common Tests
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_common.cc")
    add_executable(run_common_tests
      test_common.cc
      test_utils.hpp
    )
    target_link_libraries(run_common_tests
      PRIVATE unilink GTest::gtest_main GTest::gmock
    )
    gtest_discover_tests(run_common_tests
      PROPERTIES LABELS "legacy;unit"
    )
  endif()
  
  # Communication Tests
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_communication.cc")
    add_executable(run_communication_tests
      test_communication.cc
      test_utils.hpp
    )
    target_link_libraries(run_communication_tests
      PRIVATE unilink GTest::gtest_main GTest::gmock
    )
    gtest_discover_tests(run_communication_tests
      PROPERTIES LABELS "legacy;integration"
    )
  endif()
  
endif()

# ============================================================================
# DOCUMENTATION
# ============================================================================

message(STATUS "Test Structure:")
message(STATUS "  - unit/       : Fast unit tests (<10s)")
message(STATUS "  - integration/: Medium integration tests (<20s)")
message(STATUS "  - e2e/        : End-to-end scenarios (<30s)")
message(STATUS "  - performance/: Optional benchmarks")
message(STATUS "")
message(STATUS "Run specific test groups with:")
message(STATUS "  ctest -L unit")
message(STATUS "  ctest -L integration")
message(STATUS "  ctest -L e2e")
message(STATUS "  ctest -L performance")
