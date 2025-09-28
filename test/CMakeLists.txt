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

# Define the real communication test executable (actual server-client communication)
add_executable(run_real_communication_tests
    test_real_communication.cc
)

# Define the debug communication test executable (debugging server-client issues)
add_executable(run_debug_communication_tests
    test_debug_communication.cc
)

# Define the fixed communication test executable (corrected server-client communication)
add_executable(run_fixed_communication_tests
    test_fixed_communication.cc
)

# Define the detailed debug test executable (detailed debugging with raw sockets)
add_executable(run_detailed_debug_tests
    test_detailed_debug.cc
)

# Define the simple server test executable (basic server functionality)
add_executable(run_simple_server_tests
    test_simple_server.cc
)

# Define the IoContext fix test executable (testing with started IoContextManager)
add_executable(run_iocontext_fix_tests
    test_iocontext_fix.cc
)

# Define the improved architecture test executable (testing auto-initialization and resource management)
add_executable(run_improved_architecture_tests
    test_improved_architecture.cc
)

# Define the safe architecture test executable (safe version of architecture tests)
add_executable(run_safe_architecture_tests
    test_safe_architecture.cc
)

# Define the final architecture test executable (final version without resource cleanup issues)
add_executable(run_final_architecture_tests
    test_final_architecture.cc
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

# Link the real communication test executable (actual server-client communication)
target_link_libraries(run_real_communication_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the debug communication test executable (debugging server-client issues)
target_link_libraries(run_debug_communication_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the fixed communication test executable (corrected server-client communication)
target_link_libraries(run_fixed_communication_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the detailed debug test executable (detailed debugging with raw sockets)
target_link_libraries(run_detailed_debug_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the simple server test executable (basic server functionality)
target_link_libraries(run_simple_server_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the IoContext fix test executable (testing with started IoContextManager)
target_link_libraries(run_iocontext_fix_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the improved architecture test executable (testing auto-initialization and resource management)
target_link_libraries(run_improved_architecture_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the safe architecture test executable (safe version of architecture tests)
target_link_libraries(run_safe_architecture_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the final architecture test executable (final version without resource cleanup issues)
target_link_libraries(run_final_architecture_tests
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
gtest_discover_tests(run_real_communication_tests)
gtest_discover_tests(run_debug_communication_tests)
gtest_discover_tests(run_fixed_communication_tests)
gtest_discover_tests(run_detailed_debug_tests)
gtest_discover_tests(run_simple_server_tests)
gtest_discover_tests(run_iocontext_fix_tests)
gtest_discover_tests(run_improved_architecture_tests)
gtest_discover_tests(run_safe_architecture_tests)
gtest_discover_tests(run_final_architecture_tests)
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
