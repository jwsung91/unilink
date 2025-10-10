# Integration Tests - Component integration tests
# Each test file is built as a separate executable

# TCP integration tests
# NOTE: test_communication.cc temporarily disabled due to last_error_ issues
foreach(test_file test_integration.cc test_simple_server.cc test_client_limit_integration.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} tcp/${test_file})
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_integration_${test_name}
    PROPERTIES
      LABELS "integration;tcp;medium"
      TIMEOUT 60
  )
endforeach()

# Serial integration tests
foreach(test_file test_serial.cc test_serial_builder_improvements.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} serial/${test_file})
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_integration_${test_name}
    PROPERTIES
      LABELS "integration;serial;medium"
      TIMEOUT 60
  )
endforeach()

# Mock integration tests
foreach(test_file test_mock_integration.cc test_mock_integrated.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} mock/${test_file})
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
      ${CMAKE_SOURCE_DIR}/test/mocks
  )
  gtest_discover_tests(run_integration_${test_name}
    PROPERTIES
      LABELS "integration;mock;medium"
      TIMEOUT 60
  )
endforeach()

# General integration tests
foreach(test_file test_builder_integration.cc test_stable_integration.cc test_core_integrated.cc test_safety_integrated.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_integration_${test_name} ${test_file})
  target_link_libraries(run_integration_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  target_include_directories(run_integration_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_integration_${test_name}
    PROPERTIES
      LABELS "integration;general;medium"
      TIMEOUT 60
  )
endforeach()
