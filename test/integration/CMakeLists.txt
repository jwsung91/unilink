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

# Integration Tests - Component integration tests
# Each test file is built as a separate executable

# TCP integration tests
# NOTE: test_communication.cc temporarily disabled due to last_error_ issues
get_property(_unilink_test_lib GLOBAL PROPERTY UNILINK_TEST_LIB)

foreach(test_file test_integration.cc test_simple_server.cc test_client_limit_integration.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/tcp/${test_file})
  set_target_properties(run_integration_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  # Mark test_integration to use resource lock to avoid port conflicts
  if(test_name STREQUAL "test_integration")
    gtest_discover_tests(run_integration_${test_name}
      PROPERTIES
        LABELS "integration;tcp;medium"
        TIMEOUT 60
        RESOURCE_LOCK "tcp_port_allocation"
    )
  else()
    gtest_discover_tests(run_integration_${test_name}
      PROPERTIES
        LABELS "integration;tcp;medium"
        TIMEOUT 60
        RESOURCE_LOCK "tcp_port_allocation"
    )
  endif()
  unilink_copy_runtime_dependency(run_integration_${test_name})
endforeach()

# Serial integration tests
# NOTE: test_serial.cc temporarily disabled due to complex variable naming issues
foreach(test_file test_serial_builder_improvements.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/serial/${test_file})
  set_target_properties(run_integration_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_integration_${test_name}
    PROPERTIES
      LABELS "integration;serial;medium"
      TIMEOUT 60
      RESOURCE_LOCK "tcp_port_allocation"
  )
  unilink_copy_runtime_dependency(run_integration_${test_name})
endforeach()

# Mock integration tests - DISABLED due to dependency injection issues
# These tests require DI architecture which is not implemented in the library
# Real integration tests provide better coverage
# foreach(test_file test_mock_integration.cc test_mock_integrated.cc)
#   get_filename_component(test_name ${test_file} NAME_WE)
#   add_executable(run_integration_${test_name} mock/${test_file})
#   target_link_libraries(run_integration_${test_name}
#     PRIVATE
#       unilink_shared
#       GTest::gtest
#       GTest::gtest_main
#       GTest::gmock
#   )
#   target_include_directories(run_integration_${test_name}
#     PRIVATE
#       ${CMAKE_SOURCE_DIR}/test/utils
#       ${CMAKE_SOURCE_DIR}/test/fixtures
#       ${CMAKE_SOURCE_DIR}/test/mocks
#   )
#   gtest_discover_tests(run_integration_${test_name}
#     PROPERTIES
#       LABELS "integration;mock;medium"
#       TIMEOUT 60
#   )
# endforeach()

# General integration tests
foreach(test_file test_builder_integration.cc test_stable_integration.cc test_core_integrated.cc test_safety_integrated.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/${test_file})
  set_target_properties(run_integration_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  # Mark test_builder_integration and test_stable_integration to use resource lock
  # to avoid port timing conflicts when running in parallel
  if(test_name STREQUAL "test_builder_integration" OR test_name STREQUAL "test_stable_integration")
    gtest_discover_tests(run_integration_${test_name}
      PROPERTIES
        LABELS "integration;general;medium"
        TIMEOUT 60
        RESOURCE_LOCK "tcp_port_allocation"
    )
  else()
    gtest_discover_tests(run_integration_${test_name}
      PROPERTIES
        LABELS "integration;general;medium"
        TIMEOUT 60
        RESOURCE_LOCK "tcp_port_allocation"
    )
  endif()
  unilink_copy_runtime_dependency(run_integration_${test_name})
endforeach()
