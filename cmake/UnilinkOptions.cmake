# Unilink build options
# This file centralizes all build options for better maintainability

include(CMakeDependentOption)

# Build type options
option(UNILINK_BUILD_SHARED "Build shared library" ON)
option(UNILINK_BUILD_STATIC "Build static library" ON)
option(UNILINK_BUILD_EXAMPLES "Build examples" ON)
option(UNILINK_BUILD_TESTS "Build tests" ON)
option(UNILINK_BUILD_DOCS "Build documentation" ON)

# Granular test toggles (inherit from UNILINK_BUILD_TESTS)
cmake_dependent_option(UNILINK_ENABLE_UNIT_TESTS "Build unit tests" ON
  "UNILINK_BUILD_TESTS" OFF)
cmake_dependent_option(UNILINK_ENABLE_INTEGRATION_TESTS "Build integration tests" ON
  "UNILINK_BUILD_TESTS" OFF)
cmake_dependent_option(UNILINK_ENABLE_E2E_TESTS "Build end-to-end tests" ON
  "UNILINK_BUILD_TESTS" OFF)
cmake_dependent_option(UNILINK_ENABLE_PERFORMANCE_TESTS "Enable performance/benchmark tests" OFF
  "UNILINK_BUILD_TESTS" OFF)

# Feature options
option(UNILINK_ENABLE_CONFIG "Enable configuration management API" ON)
option(UNILINK_ENABLE_MEMORY_TRACKING "Enable memory tracking for debugging" ON)
option(UNILINK_ENABLE_SANITIZERS "Enable sanitizers in Debug builds" OFF)

# Installation options
option(UNILINK_ENABLE_INSTALL "Enable install/export targets" ON)
option(UNILINK_ENABLE_PKGCONFIG "Install pkg-config file" ON)
option(UNILINK_ENABLE_EXPORT_HEADER "Generate export header" ON)

# Compiler options
option(UNILINK_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(UNILINK_ENABLE_WERROR "Treat warnings as errors" OFF)
option(UNILINK_ENABLE_COVERAGE "Enable code coverage" OFF)

# Platform-specific options
option(UNILINK_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(UNILINK_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(UNILINK_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# Performance options
option(UNILINK_ENABLE_LTO "Enable Link Time Optimization" OFF)
option(UNILINK_ENABLE_PCH "Enable Precompiled Headers" OFF)

# Derived flag to simplify conditional test logic
set(UNILINK_ENABLE_ANY_TESTS OFF)
if(UNILINK_BUILD_TESTS AND
   (UNILINK_ENABLE_UNIT_TESTS
    OR UNILINK_ENABLE_INTEGRATION_TESTS
    OR UNILINK_ENABLE_E2E_TESTS
    OR UNILINK_ENABLE_PERFORMANCE_TESTS))
  set(UNILINK_ENABLE_ANY_TESTS ON)
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Validate build type
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel")
if(NOT CMAKE_BUILD_TYPE IN_LIST CMAKE_CONFIGURATION_TYPES)
  message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}. "
          "Valid options are: ${CMAKE_CONFIGURATION_TYPES}")
endif()

# Set default C++ standard
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
