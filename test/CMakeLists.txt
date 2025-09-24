cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Define the test executable
add_executable(run_tests
    test_common.cc
)

# Link the test executable against our library and GoogleTest
target_link_libraries(run_tests
    PRIVATE
    unilink
    GTest::gtest_main
)

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)

add_test(NAME ${PROJECT_NAME} COMMAND run_tests)
