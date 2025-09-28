cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Define the test executable
add_executable(run_tests
    test_common.cc
    test_serial.cc
    test_tcp_server.cc
    test_tcp_client.cc
    test_config_manager.cc
)

# Define the wrapper test executable
add_executable(run_wrapper_tests
    test_wrapper.cc
)

# Define the builder unit test executable (basic builder functionality + Serial support)
add_executable(run_builder_tests
    test_builder.cc
)

# Define the builder integration test executable (TCP integration tests + Serial integration)
add_executable(run_builder_integration_tests
    test_builder_integration.cc
)

# Define the improved architecture test executable (removed - integrated into main tests)

# Link the test executable against our library and GoogleTest
target_link_libraries(run_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the wrapper test executable
target_link_libraries(run_wrapper_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the builder unit test executable (basic builder functionality + Serial support)
target_link_libraries(run_builder_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the builder integration test executable (TCP integration tests + Serial integration)
target_link_libraries(run_builder_integration_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the improved architecture test executable (removed - integrated into main tests)

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)
gtest_discover_tests(run_wrapper_tests)
gtest_discover_tests(run_builder_tests)
gtest_discover_tests(run_builder_integration_tests)
# gtest_discover_tests(run_improved_tests) # removed - integrated into main tests

# Enable sanitizers for debugging memory errors and undefined behavior.
# This is typically done for Debug or RelWithDebInfo builds.
# AddressSanitizer disabled due to boost::asio internal memory management issues
if(false)
    # AddressSanitizer (ASan) for memory errors like use-after-free, buffer overflows, etc.
    target_compile_options(run_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_tests PRIVATE -fsanitize=address)
    # UndefinedBehaviorSanitizer (UBSan) for various forms of undefined behavior.
    target_compile_options(run_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to wrapper tests
    target_compile_options(run_wrapper_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_wrapper_tests PRIVATE -fsanitize=address)
    target_compile_options(run_wrapper_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_wrapper_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to builder tests
    target_compile_options(run_builder_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_builder_tests PRIVATE -fsanitize=address)
    target_compile_options(run_builder_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_builder_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to builder integration tests
    target_compile_options(run_builder_integration_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_builder_integration_tests PRIVATE -fsanitize=address)
    target_compile_options(run_builder_integration_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_builder_integration_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to improved architecture tests (removed - integrated into main tests)
endif()
