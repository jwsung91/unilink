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

# Default: discover after build so test executables exist
set(_UNILINK_GTEST_DISCOVERY_MODE POST_BUILD)

foreach(test_file test_core.cc test_memory.cc test_boundary.cc test_error_handler.cc test_input_validator.cc test_logger_advanced.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/common/${test_file})
  set_target_properties(run_unit_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
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
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/builder/${test_file})
  set_target_properties(run_unit_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
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
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/config/${test_file})
  set_target_properties(run_unit_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
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
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/${test_file})
  set_target_properties(run_unit_${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
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

# Wrapper serial config mapping
add_executable(run_unit_wrapper_serial_config_mapping ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/test_serial_config_mapping.cc)
set_target_properties(run_unit_wrapper_serial_config_mapping PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(run_unit_wrapper_serial_config_mapping
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_wrapper_serial_config_mapping)
target_include_directories(run_unit_wrapper_serial_config_mapping
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)

gtest_discover_tests(run_unit_wrapper_serial_config_mapping
  PROPERTIES
    LABELS "unit;wrapper;serial"
    TIMEOUT 30
)
unilink_copy_runtime_dependency(run_unit_wrapper_serial_config_mapping)

# Transport tests (fast, unit-level without real network dependencies)
add_executable(run_unit_transport_tcp_client ${CMAKE_CURRENT_SOURCE_DIR}/transport/transport_tcp_client.cc)
set_target_properties(run_unit_transport_tcp_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(run_unit_transport_tcp_client
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_transport_tcp_client)
target_include_directories(run_unit_transport_tcp_client
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)

gtest_discover_tests(run_unit_transport_tcp_client
  PROPERTIES
    LABELS "unit;transport;fast"
    TIMEOUT 30
)
unilink_copy_runtime_dependency(run_unit_transport_tcp_client)

# Serial lifecycle tests (fast, no real device)
add_executable(run_unit_transport_serial ${CMAKE_CURRENT_SOURCE_DIR}/transport/transport_serial.cc)
set_target_properties(run_unit_transport_serial PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(run_unit_transport_serial
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_transport_serial)
target_include_directories(run_unit_transport_serial
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)

gtest_discover_tests(run_unit_transport_serial
  PROPERTIES
    LABELS "unit;transport;serial"
    TIMEOUT 30
)
unilink_copy_runtime_dependency(run_unit_transport_serial)

# Transport layer tests removed - they require actual network/serial devices

# Create a custom target to build all unit tests
add_custom_target(unilink_unit_tests)

# Add dependencies to the custom target
add_dependencies(unilink_unit_tests
  run_unit_test_core
  run_unit_test_memory
  run_unit_test_boundary
  run_unit_test_error_handler
  run_unit_test_input_validator
  run_unit_test_logger_advanced
  run_unit_test_builder
  run_unit_test_config
  run_unit_test_tcp_server_advanced
  run_unit_test_tcp_client_advanced
  run_unit_wrapper_serial_config_mapping
  run_unit_transport_tcp_client
  run_unit_transport_serial
)
