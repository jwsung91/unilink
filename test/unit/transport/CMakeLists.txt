# Transport layer unit tests
add_executable(run_unit_test_transport_tcp_client_error
  test_tcp_client_error_cases.cc
)

add_executable(run_unit_test_transport_tcp_server_error
  test_tcp_server_error_cases.cc
)

add_executable(run_unit_test_transport_serial_error
  test_serial_error_cases.cc
)

add_executable(run_unit_test_transport_scenarios
  test_transport_scenarios.cc
)

# Link libraries
target_link_libraries(run_unit_test_transport_tcp_client_error
  PRIVATE
  unilink_shared
  gtest
  gtest_main
)

target_link_libraries(run_unit_test_transport_tcp_server_error
  PRIVATE
  unilink_shared
  gtest
  gtest_main
)

target_link_libraries(run_unit_test_transport_serial_error
  PRIVATE
  unilink_shared
  gtest
  gtest_main
)

target_link_libraries(run_unit_test_transport_scenarios
  PRIVATE
  unilink_shared
  gtest
  gtest_main
)

# Add tests to CTest
add_test(NAME TcpClientErrorTest
  COMMAND run_unit_test_transport_tcp_client_error
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(NAME TcpServerErrorTest
  COMMAND run_unit_test_transport_tcp_server_error
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(NAME SerialErrorTest
  COMMAND run_unit_test_transport_serial_error
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(NAME TransportScenarioTest
  COMMAND run_unit_test_transport_scenarios
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Set test labels
set_tests_properties(TcpClientErrorTest PROPERTIES LABELS "unit;transport;tcp;error")
set_tests_properties(TcpServerErrorTest PROPERTIES LABELS "unit;transport;tcp;error")
set_tests_properties(SerialErrorTest PROPERTIES LABELS "unit;transport;serial;error")
set_tests_properties(TransportScenarioTest PROPERTIES LABELS "unit;transport;scenario")
