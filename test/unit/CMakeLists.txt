# Unit Tests - Fast, isolated tests
# Each test file is built as a separate executable due to having its own main()

# Common tests (separate executables)
# NOTE: test_common.cc temporarily disabled due to namespace issues
foreach(test_file test_core.cc test_memory.cc test_boundary.cc test_error_handler.cc test_input_validator.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} common/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
  target_include_directories(run_unit_${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/test/utils
      ${CMAKE_SOURCE_DIR}/test/fixtures
  )
  gtest_discover_tests(run_unit_${test_name}
    PROPERTIES
      LABELS "unit;common;fast"
      TIMEOUT 30
  )
endforeach()

# Builder tests (separate executables)
foreach(test_file test_builder.cc test_builder_coverage.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} builder/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
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
endforeach()

# Config tests (separate executables)
foreach(test_file test_config.cc test_config_coverage.cc)
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(run_unit_${test_name} config/${test_file})
  target_link_libraries(run_unit_${test_name}
    PRIVATE
      unilink
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
  )
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
endforeach()
