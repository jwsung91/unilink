cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Define the test executable
add_executable(run_tests
    test_common.cc
    test_serial.cc
    test_tcp_server.cc
)

# Link the test executable against our library and GoogleTest
target_link_libraries(run_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)

# Enable sanitizers for debugging memory errors and undefined behavior.
# This is typically done for Debug or RelWithDebInfo builds.
if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    # AddressSanitizer (ASan) for memory errors like use-after-free, buffer overflows, etc.
    target_compile_options(run_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_tests PRIVATE -fsanitize=address)
    # UndefinedBehaviorSanitizer (UBSan) for various forms of undefined behavior.
    target_compile_options(run_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_tests PRIVATE -fsanitize=undefined)
endif()
