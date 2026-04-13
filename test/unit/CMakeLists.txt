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
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Default: discover after build so test executables exist
set(_UNILINK_GTEST_DISCOVERY_MODE POST_BUILD)

foreach(test_file test_core.cc test_memory.cc test_pooled_buffer.cc test_boundary.cc test_error_handler.cc test_input_validator.cc test_input_validator_edge.cc test_logger_advanced.cc test_pool_stress.cc test_error_mapping.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/common/${test_file})
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
        LABELS "unit_common_fast"
        TIMEOUT 30
      RESOURCE_LOCK "log_file_io"
    )
  else()
    gtest_discover_tests(run_unit_${test_name}
      PROPERTIES
        LABELS "unit_common_fast"
        TIMEOUT 30
    )
  endif()
endforeach()

# Builder tests (separate executables)
foreach(test_file test_builder.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/builder/${test_file})
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
      LABELS "unit_builder_fast"
      TIMEOUT 30
  )
endforeach()

# Config tests (separate executables)
foreach(test_file test_config.cc test_config_security.cc test_config_failure.cpp)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/config/${test_file})
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
      LABELS "unit_config_fast"
      TIMEOUT 30
  )
endforeach()

# Memory tests (separate executables)
foreach(test_file test_pool_limits.cpp)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/memory/${test_file})
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
      TIMEOUT 30
  )
endforeach()

# Wrapper tests (separate executables across TCP/UDP/UDS/Serial)
foreach(test_file test_tcp_server_advanced.cc test_tcp_client_advanced.cc test_serial_advanced.cc test_udp_stop_perf.cc test_udp_options.cc test_udp_failure.cc test_udp_state.cpp test_tcp_client_error_mapping.cc test_uds_advanced.cc test_udp_server_advanced.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/${test_file})
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
      LABELS "unit_wrapper"
      TIMEOUT 120
      RESOURCE_LOCK "tcp_port_allocation"
  )
endforeach()

# Wrapper serial config mapping
add_executable(run_unit_wrapper_serial_config_mapping ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/test_serial_config_mapping.cc)
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
    LABELS "unit_wrapper_serial"
    TIMEOUT 30
)

# Documentation snippet compile smoke tests
set(_unilink_doc_snippet_dir ${CMAKE_CURRENT_BINARY_DIR}/generated/doc_snippets)
set(_unilink_doc_extract_script ${CMAKE_SOURCE_DIR}/docs/scripts/extract_doc_snippet.py)
set(_unilink_doc_snippets
  "tutorial_getting_started_client|docs/tutorials/01_getting_started.md"
  "tutorial_tcp_server_echo|docs/tutorials/02_tcp_server.md"
  "tutorial_uds_server|docs/tutorials/03_uds_communication.md"
  "tutorial_uds_client|docs/tutorials/03_uds_communication.md"
  "tutorial_serial_terminal|docs/tutorials/04_serial_communication.md"
  "tutorial_udp_receiver|docs/tutorials/05_udp_communication.md"
  "tutorial_udp_sender|docs/tutorials/05_udp_communication.md"
)

set(_unilink_doc_targets)
foreach(doc_entry IN LISTS _unilink_doc_snippets)
  string(REPLACE "|" ";" doc_entry_parts "${doc_entry}")
  list(GET doc_entry_parts 0 doc_snippet_name)
  list(GET doc_entry_parts 1 doc_source_relpath)

  set(doc_source_path ${CMAKE_SOURCE_DIR}/${doc_source_relpath})
  set(doc_generated_source ${_unilink_doc_snippet_dir}/${doc_snippet_name}.cc)
  set(doc_target run_doc_snippet_${doc_snippet_name})

  add_custom_command(
    OUTPUT ${doc_generated_source}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_unilink_doc_snippet_dir}
    COMMAND
      ${Python3_EXECUTABLE}
      ${_unilink_doc_extract_script}
      --input ${doc_source_path}
      --snippet ${doc_snippet_name}
      --output ${doc_generated_source}
    DEPENDS ${doc_source_path} ${_unilink_doc_extract_script}
    VERBATIM
  )

  add_executable(${doc_target} ${doc_generated_source})
  target_link_libraries(${doc_target} PRIVATE ${_unilink_test_lib})
  target_include_directories(${doc_target} PRIVATE ${CMAKE_SOURCE_DIR})
  target_compile_features(${doc_target} PRIVATE cxx_std_17)
  unilink_set_test_warning_options(${doc_target})

  list(APPEND _unilink_doc_targets ${doc_target})
endforeach()

add_custom_target(doc_snippets_smoke DEPENDS ${_unilink_doc_targets})
add_test(
  NAME DocSnippetsCompile
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target doc_snippets_smoke --config $<CONFIG>
)
set_tests_properties(
  DocSnippetsCompile
  PROPERTIES
    LABELS "docs_snippets"
    TIMEOUT 120
)

# Transport tests (fast, unit-level without real network dependencies)
# Updated list to include fuzz tests, server session tests, and timeout tests
foreach(test_file transport_tcp_client.cc transport_tcp_server.cc transport_uds_client.cc transport_uds_server.cc transport_serial.cc transport_udp.cc transport_udp_extended.cc transport_tcp_client_policy.cc reconnect_logic_test.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/transport/${test_file})
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
      LABELS "unit_transport_fast"
      TIMEOUT 30
  )
endforeach()

# Deterministic reconnect decision tests (policy/max-retries/clamp logic)
add_executable(run_unit_test_reconnect_decider ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_reconnect_decider.cc)
target_link_libraries(run_unit_test_reconnect_decider
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_test_reconnect_decider)
target_include_directories(run_unit_test_reconnect_decider
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_test_reconnect_decider
  PROPERTIES
    LABELS "stable"
    TIMEOUT 30
)

# Contract Compliance Tests
add_executable(run_unit_test_contract_compliance ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_contract_compliance.cc)
target_link_libraries(run_unit_test_contract_compliance
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_test_contract_compliance)
target_include_directories(run_unit_test_contract_compliance
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)

gtest_discover_tests(run_unit_test_contract_compliance
  PROPERTIES
    LABELS "unit_transport_contract"
    TIMEOUT 30
)

# New error handling tests
add_executable(run_unit_test_tcp_abort ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_tcp_abort.cc)
target_link_libraries(run_unit_test_tcp_abort
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_test_tcp_abort)
target_include_directories(run_unit_test_tcp_abort
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_test_tcp_abort
  PROPERTIES
    LABELS "unit_transport_error"
    TIMEOUT 30
)

add_executable(run_unit_test_tcp_rst ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_tcp_rst.cpp)
target_link_libraries(run_unit_test_tcp_rst
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_test_tcp_rst)
target_include_directories(run_unit_test_tcp_rst
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_test_tcp_rst
  PROPERTIES
    LABELS "unit_transport_error"
    TIMEOUT 30
)

add_executable(run_unit_test_udp_error ${CMAKE_CURRENT_SOURCE_DIR}/transport/test_udp_error.cc)
target_link_libraries(run_unit_test_udp_error
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_test_udp_error)
target_include_directories(run_unit_test_udp_error
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_test_udp_error
  PROPERTIES
    LABELS "unit_transport_error"
    TIMEOUT 30
)

add_executable(run_unit_test_serial_fail ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/test_serial_fail.cc)
target_link_libraries(run_unit_test_serial_fail
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_test_serial_fail)
target_include_directories(run_unit_test_serial_fail
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_test_serial_fail
  PROPERTIES
    LABELS "unit_wrapper_error"
    TIMEOUT 30
)

# Transport TCP Server Session Test
add_executable(run_unit_transport_tcp_server_session ${CMAKE_CURRENT_SOURCE_DIR}/transport/transport_tcp_server_session.cc)
target_link_libraries(run_unit_transport_tcp_server_session
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_transport_tcp_server_session)
target_include_directories(run_unit_transport_tcp_server_session
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_transport_tcp_server_session
  PROPERTIES
    LABELS "unit_transport_session"
    TIMEOUT 30
)

# Transport TCP Fuzz Test
add_executable(run_unit_transport_tcp_fuzz_test ${CMAKE_CURRENT_SOURCE_DIR}/transport/transport_tcp_fuzz_test.cc)
target_link_libraries(run_unit_transport_tcp_fuzz_test
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_transport_tcp_fuzz_test)
target_include_directories(run_unit_transport_tcp_fuzz_test
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_transport_tcp_fuzz_test
  PROPERTIES
    LABELS "unit_transport_fuzz"
    TIMEOUT 30
)

# Transport TCP Timeout Test
add_executable(run_unit_transport_tcp_timeout_test ${CMAKE_CURRENT_SOURCE_DIR}/transport/transport_tcp_timeout_test.cc)
target_link_libraries(run_unit_transport_tcp_timeout_test
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_transport_tcp_timeout_test)
target_include_directories(run_unit_transport_tcp_timeout_test
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_transport_tcp_timeout_test
  PROPERTIES
    LABELS "unit_transport_timeout"
    TIMEOUT 30
)

# Transport TCP Server Security Test
add_executable(run_unit_transport_tcp_server_security_test ${CMAKE_CURRENT_SOURCE_DIR}/transport/transport_tcp_server_security_test.cc)
target_link_libraries(run_unit_transport_tcp_server_security_test
  PRIVATE
    ${_unilink_test_lib}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
unilink_set_test_warning_options(run_unit_transport_tcp_server_security_test)
target_include_directories(run_unit_transport_tcp_server_security_test
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_unit_transport_tcp_server_security_test
  PROPERTIES
    LABELS "unit_transport_security"
    TIMEOUT 30
)

# Framer tests (separate executables)
foreach(test_file line_framer_test.cc packet_framer_test.cc line_framer_security_test.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/framer/${test_file})
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
      LABELS "unit_framer"
      TIMEOUT 30
  )
endforeach()

# Framer tests (separate executables)
add_custom_target(unilink_unit_tests)

# Add dependencies to the custom target
add_dependencies(unilink_unit_tests
  run_unit_test_core
  run_unit_test_memory
  run_unit_test_pool_limits
  run_unit_test_pool_stress
  run_unit_test_boundary
  run_unit_test_error_handler
  run_unit_test_input_validator
  run_unit_test_logger_advanced
  run_unit_test_builder
  run_unit_test_config
  run_unit_test_tcp_server_advanced
  run_unit_test_tcp_client_advanced
  run_unit_test_udp_options
  run_unit_test_udp_state
  run_unit_test_tcp_client_error_mapping
  run_unit_test_error_mapping
  run_unit_wrapper_serial_config_mapping
  run_unit_transport_tcp_client
  run_unit_transport_tcp_server
  run_unit_transport_uds_client
  run_unit_transport_uds_server
  run_unit_transport_serial
  run_unit_transport_udp_extended
  run_unit_transport_tcp_server_session
  run_unit_transport_tcp_fuzz_test
  run_unit_transport_tcp_timeout_test
  run_unit_test_contract_compliance
  run_unit_test_tcp_abort
  run_unit_test_tcp_rst
  run_unit_test_udp_error
  run_unit_test_serial_fail
  run_unit_test_config_security
  run_unit_test_config_failure
  run_unit_transport_tcp_server_session
  run_unit_transport_tcp_fuzz_test
  run_unit_transport_tcp_timeout_test
  run_unit_transport_tcp_server_security_test
  run_unit_line_framer_security_test
  run_unit_transport_tcp_client_policy
  run_unit_test_reconnect_decider
)
