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

# Unit Tests - Fast, isolated tests
# Each test file is built as a separate executable due to having its own main()

get_property(_unilink_test_lib GLOBAL PROPERTY UNILINK_TEST_LIB)

foreach(test_file test_core.cc test_memory.cc test_boundary.cc test_error_handler.cc test_input_validator.cc test_logger_advanced.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} common/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_unit_${test_name})
  target_include_directories(run_unit_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  # Mark test_core tests to use resource lock to avoid file I/O conflicts
  if(test_name STREQUAL "test_core")
    gtest_discover_tests(run_unit_${test_name}
      PROPERTIES
        LABELS "unit;common;fast"
        TIMEOUT 30
      RESOURCE_LOCK "log_file_io"
    )
  else()
    gtest_discover_tests(run_unit_${test_name}
      PROPERTIES
        LABELS "unit;common;fast"
        TIMEOUT 30
    )
  endif()
  unilink_copy_runtime_dependency(run_unit_${test_name})
endforeach()

# Builder tests (separate executables)
foreach(test_file test_builder.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} builder/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_unit_${test_name})
  target_include_directories(run_unit_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_unit_${test_name}
    PROPERTIES
      LABELS "unit;builder;fast"
      TIMEOUT 30
  )
  unilink_copy_runtime_dependency(run_unit_${test_name})
endforeach()

# Config tests (separate executables)
foreach(test_file test_config.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} config/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_unit_${test_name})
  target_include_directories(run_unit_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_unit_${test_name}
    PROPERTIES
      LABELS "unit;config;fast"
      TIMEOUT 30
  )
  unilink_copy_runtime_dependency(run_unit_${test_name})
endforeach()

# Wrapper tests (separate executables)
foreach(test_file test_tcp_server_advanced.cc test_tcp_client_advanced.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} wrapper/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      ${_unilink_test_lib}
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  unilink_set_test_warning_options(run_unit_${test_name})
  target_include_directories(run_unit_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_unit_${test_name}
    PROPERTIES
      LABELS "unit;wrapper;tcp;coverage"
      TIMEOUT 60
      RESOURCE_LOCK "tcp_port_allocation"
  )
  unilink_copy_runtime_dependency(run_unit_${test_name})
endforeach()

# Transport layer tests removed - they require actual network/serial devices
