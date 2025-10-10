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
option(UNILINK_ENABLE_PERFORMANCE_TESTS "Enable performance/benchmark tests (slower)" OFF)
option(BUILD_DOCUMENTATION "Build documentation with Doxygen" ON)

if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  
  # Prevent GoogleTest from overriding our compiler/linker options
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  set(gtest_build_tests OFF CACHE BOOL "" FORCE)
  set(gtest_build_samples OFF CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(googletest)
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

# Documentation generation with Doxygen
if(BUILD_DOCUMENTATION)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    # Create docs directory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
    
    # Configure Doxygen
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/config/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/docs/config/Doxyfile)
    
    # Add custom target for documentation
    add_custom_target(docs
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/config/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
    
    # Add install target for documentation
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/html/
      DESTINATION share/doc/unilink/html
      OPTIONAL
    )
    
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    message(STATUS "Documentation target 'docs' available")
  else()
    message(WARNING "Doxygen not found. Documentation will not be generated.")
    message(STATUS "Install Doxygen to enable documentation generation:")
    message(STATUS "  Ubuntu/Debian: sudo apt install doxygen")
    message(STATUS "  CentOS/RHEL: sudo yum install doxygen")
  endif()
endif()
