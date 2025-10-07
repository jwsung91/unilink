cmake_minimum_required(VERSION 3.10)
project(unilink-test)

# Enable configuration management API for tests
target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_CONFIG=1)

# Define the core test executable
add_executable(run_tests
    test_common.cc
    test_config_manager.cc
)

# Define the transport performance test executable (focused unit tests for critical performance aspects)
add_executable(run_transport_performance_tests
    test_transport_performance.cc
)

# Define the builder unit test executable (basic builder functionality + Serial support)
add_executable(run_builder_tests
    test_builder.cc
)

# Define the builder integration test executable (TCP integration tests + Serial integration)
add_executable(run_builder_integration_tests
    test_builder_integration.cc
)

# Define the simple server test executable (basic server functionality)
add_executable(run_simple_server_tests
    test_simple_server.cc
)

# Define the integrated architecture test executable (all architecture tests combined)
add_executable(run_architecture_tests
    test_architecture.cc
)

# Define the integrated communication test executable (all communication tests combined)
add_executable(run_communication_tests
    test_communication.cc
)

# Define the memory pool test executable
add_executable(run_memory_pool_tests
    test_memory_pool.cc
)

# Note: IoContext management tests are now integrated into run_builder_tests

# Link the core test executable against our library and GoogleTest
target_link_libraries(run_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the transport performance test executable
target_link_libraries(run_transport_performance_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the builder unit test executable
target_link_libraries(run_builder_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the builder integration test executable
target_link_libraries(run_builder_integration_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the simple server test executable
target_link_libraries(run_simple_server_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the integrated architecture test executable
target_link_libraries(run_architecture_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the integrated communication test executable
target_link_libraries(run_communication_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Link the memory pool test executable
target_link_libraries(run_memory_pool_tests
    PRIVATE
    unilink
    GTest::gtest_main
    GTest::gmock
)

# Note: IoContext management tests are now integrated into run_builder_tests

# Discover and add tests to CTest
include(GoogleTest)
gtest_discover_tests(run_tests)
gtest_discover_tests(run_transport_performance_tests)
gtest_discover_tests(run_builder_tests)
gtest_discover_tests(run_builder_integration_tests)
gtest_discover_tests(run_simple_server_tests)
gtest_discover_tests(run_architecture_tests)
gtest_discover_tests(run_communication_tests)
gtest_discover_tests(run_memory_pool_tests)
# Note: IoContext management tests are now integrated into run_builder_tests

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
    
    # Apply same sanitizers to architecture tests
    target_compile_options(run_architecture_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_architecture_tests PRIVATE -fsanitize=address)
    target_compile_options(run_architecture_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_architecture_tests PRIVATE -fsanitize=undefined)
    
    # Apply same sanitizers to communication tests
    target_compile_options(run_communication_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(run_communication_tests PRIVATE -fsanitize=address)
    target_compile_options(run_communication_tests PRIVATE -fsanitize=undefined)
    target_link_options(run_communication_tests PRIVATE -fsanitize=undefined)
endif()