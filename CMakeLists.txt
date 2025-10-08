cmake_minimum_required(VERSION 3.10)
project(unilink)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(unilink)

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(UNILINK_ENABLE_CONFIG "Enable configuration management API" ON)
option(UNILINK_ENABLE_MEMORY_TRACKING "Enable memory tracking for debugging" ON)

if(UNILINK_ENABLE_CONFIG)
  target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_CONFIG=1)
endif()

if(UNILINK_ENABLE_MEMORY_TRACKING)
  target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_MEMORY_TRACKING=1)
endif()

option(BUILD_TESTING "Build tests" ON)
option(UNILINK_ENABLE_SANITIZERS "Enable AddressSanitizer and other sanitizers" OFF)

if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    GTest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip # release-1.10.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(GTest)
  add_subdirectory(test)
endif()

# Sanitizer support
if(UNILINK_ENABLE_SANITIZERS)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(unilink PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak)
    target_link_options(unilink PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak)
    
    # Set environment variables for AddressSanitizer
    set(ENV{ASAN_OPTIONS} "detect_leaks=1:abort_on_error=1:check_initialization_order=1")
    set(ENV{UBSAN_OPTIONS} "print_stacktrace=1:abort_on_error=1")
    
    message(STATUS "AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer enabled")
  else()
    message(WARNING "Sanitizers are only supported with GCC and Clang")
  endif()
endif()
