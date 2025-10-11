# End-to-End Tests - Full system tests
# Each test file is built as a separate executable

# Scenario tests
foreach(test_file test_architecture.cc test_error_recovery.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_e2e_${test_name} scenarios/${test_file})
  target_link_libraries(run_e2e_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gmock
  )
  target_include_directories(run_e2e_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_e2e_${test_name}
    PROPERTIES
      LABELS "e2e;scenario;slow"
      TIMEOUT 90
  )
endforeach()

# Stress tests
add_executable(run_e2e_test_stress stress/test_stress.cc)
target_link_libraries(run_e2e_test_stress
  PRIVATE
    unilink
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
target_include_directories(run_e2e_test_stress
  PRIVATE
    ${CMAKE_SOURCE_DIR}/test/utils
    ${CMAKE_SOURCE_DIR}/test/fixtures
)
gtest_discover_tests(run_e2e_test_stress
  PROPERTIES
    LABELS "e2e;stress;slow"
    TIMEOUT 120
)
