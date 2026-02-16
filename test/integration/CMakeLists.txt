# Copyright 2025 Jinwoo Sung
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

# Integration Tests - Component integration tests Each test file is built as a
# separate executable

# TCP integration tests NOTE: test_communication.cc temporarily disabled due to
# last_error_ issues
get_property(_unilink_test_lib GLOBAL PROPERTY UNILINK_TEST_LIB)

foreach(test_file
        test_integration.cc test_simple_server.cc
        test_client_limit_integration.cc test_multithread_backpressure.cc
        test_stop_from_callback.cc test_start_stop_stress.cc
        test_stop_contract.cc
        test_dos_protection.cc
        test_tcp_server_chaos.cc
        test_tcp_flood.cc
        test_tcp_abort.cc
        test_tcp_cancel.cc
)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(
    run_integration_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/tcp/${test_file}
  )
  target_link_libraries(
    run_integration_${test_name} PRIVATE ${_unilink_test_lib} GTest::gtest
                                         GTest::gtest_main GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(
    run_integration_${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/test/utils
                                         ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  # Mark test_integration to use resource lock to avoid port conflicts
  if(test_name STREQUAL "test_integration")
    gtest_discover_tests(
      run_integration_${test_name}
      PROPERTIES LABELS "integration_tcp_medium"
      TIMEOUT 300 RESOURCE_LOCK "tcp_port_allocation"
    )
  else()
    gtest_discover_tests(
      run_integration_${test_name}
      PROPERTIES LABELS "integration_tcp_medium"
      TIMEOUT 300 RESOURCE_LOCK "tcp_port_allocation"
    )
  endif()
endforeach()

# Serial integration tests NOTE: test_serial.cc temporarily disabled due to
# complex variable naming issues
foreach(test_file test_serial_builder_improvements.cc test_serial_loopback.cc test_serial_timeout.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(
    run_integration_${test_name}
    ${CMAKE_CURRENT_SOURCE_DIR}/serial/${test_file}
  )
  target_link_libraries(
    run_integration_${test_name} PRIVATE ${_unilink_test_lib} GTest::gtest
                                         GTest::gtest_main GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(
    run_integration_${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/test/utils
                                         ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(
    run_integration_${test_name}
    PROPERTIES LABELS "integration_serial_medium"
    TIMEOUT 300 RESOURCE_LOCK "tcp_port_allocation"
  )
endforeach()

# Builder integration tests
foreach(test_file test_builder_integration.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(
    run_integration_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/builder/${test_file}
  )
  target_link_libraries(
    run_integration_${test_name} PRIVATE ${_unilink_test_lib} GTest::gtest
                                         GTest::gtest_main GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(
    run_integration_${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/test/utils
                                         ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  # Mark test_builder_integration to use resource lock
  gtest_discover_tests(
    run_integration_${test_name}
    PROPERTIES LABELS "integration_builder_medium"
    TIMEOUT 300 RESOURCE_LOCK "tcp_port_allocation"
  )
endforeach()

# Core/General integration tests
foreach(test_file test_stable_integration.cc
                  test_core_integrated.cc test_safety_integrated.cc
)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(
    run_integration_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/core/${test_file}
  )
  target_link_libraries(
    run_integration_${test_name} PRIVATE ${_unilink_test_lib} GTest::gtest
                                         GTest::gtest_main GTest::gmock
  )
  unilink_set_test_warning_options(run_integration_${test_name})
  target_include_directories(
    run_integration_${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/test/utils
                                         ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  # Mark test_stable_integration to use resource lock
  # Increased timeout to 600s to accommodate code coverage overhead
  if(test_name STREQUAL "test_stable_integration")
    gtest_discover_tests(
      run_integration_${test_name}
      PROPERTIES LABELS "integration_core_medium"
      TIMEOUT 600 RESOURCE_LOCK "tcp_port_allocation"
    )
  else()
    gtest_discover_tests(
      run_integration_${test_name}
      PROPERTIES LABELS "integration_core_medium"
      TIMEOUT 600 RESOURCE_LOCK "tcp_port_allocation"
    )
  endif()
endforeach()
