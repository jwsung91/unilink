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

cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Enable configuration management API for tests
# Note: This is handled by the main CMakeLists.txt now

# ============================================================================
# NEW ORGANIZED TEST STRUCTURE
# ============================================================================

# Determine which unilink library to link against for tests
if(TARGET unilink_static)
  set(_unilink_test_lib unilink_static)
  set(_unilink_test_lib_is_shared OFF)
elseif(TARGET unilink_shared)
  set(_unilink_test_lib unilink_shared)
  set(_unilink_test_lib_is_shared ON)
elseif(TARGET unilink)
  set(_unilink_test_lib unilink)
  get_target_property(_unilink_target_type unilink TYPE)
  if(_unilink_target_type STREQUAL "SHARED_LIBRARY")
    set(_unilink_test_lib_is_shared ON)
  else()
    set(_unilink_test_lib_is_shared OFF)
  endif()
else()
  message(FATAL_ERROR "No unilink library target found for tests")
endif()

# Expose chosen library to subdirectories and helper functions
set_property(GLOBAL PROPERTY UNILINK_TEST_LIB ${_unilink_test_lib})
set_property(GLOBAL PROPERTY UNILINK_TEST_LIB_IS_SHARED ${_unilink_test_lib_is_shared})

# Helper to copy the shared library next to a test executable on Windows
function(unilink_copy_runtime_dependency target)
  get_property(_test_lib GLOBAL PROPERTY UNILINK_TEST_LIB)
  get_property(_is_shared GLOBAL PROPERTY UNILINK_TEST_LIB_IS_SHARED)
  if(WIN32 AND _is_shared)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${_test_lib}>
        $<TARGET_FILE_DIR:${target}>)
  endif()
endfunction()

# Include GoogleTest for test discovery
include(GoogleTest)

# Add subdirectories with organized tests (guarded by per-suite toggles)
if(UNILINK_ENABLE_UNIT_TESTS)
  message(STATUS "Building Unit Tests")
  add_subdirectory(unit)
else()
  message(STATUS "Unit Tests disabled (use -DUNILINK_ENABLE_UNIT_TESTS=ON to enable)")
endif()

if(UNILINK_ENABLE_INTEGRATION_TESTS)
  message(STATUS "Building Integration Tests")
  add_subdirectory(integration)
else()
  message(STATUS "Integration Tests disabled (use -DUNILINK_ENABLE_INTEGRATION_TESTS=ON to enable)")
endif()

if(UNILINK_ENABLE_E2E_TESTS)
  message(STATUS "Building E2E Tests")
  add_subdirectory(e2e)
else()
  message(STATUS "E2E Tests disabled (use -DUNILINK_ENABLE_E2E_TESTS=ON to enable)")
endif()

# Performance tests are optional (slower)
if(UNILINK_ENABLE_PERFORMANCE_TESTS)
  message(STATUS "Building Performance Tests (enabled)")
  add_subdirectory(performance)
else()
  message(STATUS "Performance Tests disabled (use -DUNILINK_ENABLE_PERFORMANCE_TESTS=ON to enable)")
endif()

# ============================================================================
# LEGACY TESTS - To be migrated or removed
# ============================================================================

# The following tests are kept temporarily for backward compatibility.
# They will be removed once all tests are verified in the new structure.
if(UNILINK_ENABLE_UNIT_TESTS OR UNILINK_ENABLE_INTEGRATION_TESTS OR UNILINK_ENABLE_E2E_TESTS)
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
        PRIVATE ${_unilink_test_lib} GTest::gtest_main GTest::gmock
      )
      gtest_discover_tests(run_architecture_tests
        PROPERTIES LABELS "legacy;e2e"
      )
      unilink_copy_runtime_dependency(run_architecture_tests)
    endif()
    
    # Builder Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_builder.cc")
      add_executable(run_builder_tests
        test_builder.cc
        test_utils.hpp
      )
      target_link_libraries(run_builder_tests
        PRIVATE ${_unilink_test_lib} GTest::gtest_main GTest::gmock
      )
      gtest_discover_tests(run_builder_tests
        PROPERTIES LABELS "legacy;unit"
      )
      unilink_copy_runtime_dependency(run_builder_tests)
    endif()
    
    # Common Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_common.cc")
      add_executable(run_common_tests
        test_common.cc
        test_utils.hpp
      )
      target_link_libraries(run_common_tests
        PRIVATE ${_unilink_test_lib} GTest::gtest_main GTest::gmock
      )
      gtest_discover_tests(run_common_tests
        PROPERTIES LABELS "legacy;unit"
      )
      unilink_copy_runtime_dependency(run_common_tests)
    endif()
    
    # Communication Tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_communication.cc")
      add_executable(run_communication_tests
        test_communication.cc
        test_utils.hpp
      )
      target_link_libraries(run_communication_tests
        PRIVATE ${_unilink_test_lib} GTest::gtest_main GTest::gmock
      )
      gtest_discover_tests(run_communication_tests
        PROPERTIES LABELS "legacy;integration"
      )
      unilink_copy_runtime_dependency(run_communication_tests)
    endif()

  endif()
endif()

# ============================================================================
# DOCUMENTATION
# ============================================================================

message(STATUS "")
message(STATUS "==== Test Structure ====")
message(STATUS "  - unit/       : Fast unit tests (~5s with -j)")
message(STATUS "  - integration/: Medium integration tests (~5s with -j)")
message(STATUS "  - e2e/        : End-to-end scenarios (~3s with -j)")
message(STATUS "  - performance/: Optional benchmarks (~30s with -j, disabled by default)")
message(STATUS "")
message(STATUS "Run specific test groups with:")
message(STATUS "  ctest -j$(nproc) -L unit         # Fast feedback")
message(STATUS "  ctest -j$(nproc) -L integration  # Integration tests")
message(STATUS "  ctest -j$(nproc) -LE performance # All except performance (default)")
message(STATUS "  ctest -j$(nproc)                 # All tests")
message(STATUS "")
message(STATUS "Enable performance tests:")
message(STATUS "  cmake -DUNILINK_ENABLE_PERFORMANCE_TESTS=ON ..")
message(STATUS "========================")
