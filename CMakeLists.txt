# Copyright 2025 Jinwoo Sung
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.12)
project(unilink
  VERSION 0.1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build options
option(UNILINK_BUILD_SHARED "Build shared library" ON)
option(UNILINK_ENABLE_WARNINGS "Enable warnings" ON)
option(UNILINK_ENABLE_INSTALL "Enable install/export targets" ON)
option(UNILINK_ENABLE_PKGCONFIG "Install pkg-config file" ON)
option(UNILINK_ENABLE_EXPORT_HEADER "Generate export header" ON)
option(UNILINK_ENABLE_SANITIZERS "Enable sanitizers in Debug" OFF)

# Legacy options (maintained for compatibility)
option(BUILD_EXAMPLES "Build examples" ON)
option(UNILINK_ENABLE_CONFIG "Enable configuration management API" ON)
option(UNILINK_ENABLE_MEMORY_TRACKING "Enable memory tracking for debugging" ON)
option(BUILD_TESTING "Build tests" ON)
option(UNILINK_ENABLE_PERFORMANCE_TESTS "Enable performance/benchmark tests (slower)" OFF)
option(BUILD_DOCUMENTATION "Build documentation with Doxygen" ON)

# Compiler-specific options
if(NOT MSVC)
  add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
endif()

if(UNILINK_ENABLE_WARNINGS)
  if(MSVC)
    add_compile_options(/W4 /permissive-)
  else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
  endif()
endif()

# Source files
file(GLOB_RECURSE UNILINK_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/*.cc"
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/*/*.cc")

# Public headers (for CMake target)
file(GLOB_RECURSE UNILINK_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/*.h")

# Create the library
add_library(unilink ${UNILINK_SOURCES} ${UNILINK_HEADERS})
add_library(unilink::unilink ALIAS unilink)

# Set target properties
set_target_properties(unilink PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  OUTPUT_NAME "unilink"
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
  WINDOWS_EXPORT_ALL_SYMBOLS $<IF:$<BOOL:${UNILINK_BUILD_SHARED}>,ON,OFF>)

target_compile_features(unilink PUBLIC cxx_std_17)

# Include directories
target_include_directories(unilink
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unilink)

# Find and link dependencies
find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(unilink PUBLIC Boost::system Threads::Threads)

# Configuration options
if(UNILINK_ENABLE_CONFIG)
  target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_CONFIG=1)
endif()

if(UNILINK_ENABLE_MEMORY_TRACKING)
  target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_MEMORY_TRACKING=1)
endif()

# Export header generation
if(UNILINK_ENABLE_EXPORT_HEADER)
  include(GenerateExportHeader)
  generate_export_header(unilink
    EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/unilink_export.hpp"
    BASE_NAME UNILINK)
  target_include_directories(unilink PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
endif()

# Sanitizer support
if(UNILINK_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES "Debug")
  if(NOT MSVC)
    target_compile_options(unilink PRIVATE -fsanitize=address,undefined)
    target_link_options(unilink PRIVATE -fsanitize=address,undefined)
  endif()
endif()

# Install configuration
include(GNUInstallDirs)
if(UNILINK_ENABLE_INSTALL)
  install(TARGETS unilink
    EXPORT unilinkTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unilink/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unilink
          FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
      cmake/unilinkConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/unilinkConfig.cmake
      INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unilink)
  write_basic_package_version_file(
      ${CMAKE_CURRENT_BINARY_DIR}/unilinkConfigVersion.cmake
      VERSION ${PROJECT_VERSION}
      COMPATIBILITY SameMajorVersion)

  install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/unilinkConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/unilinkConfigVersion.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unilink)

  install(EXPORT unilinkTargets
      NAMESPACE unilink::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unilink)
endif()

# pkg-config support
if(UNILINK_ENABLE_INSTALL AND UNILINK_ENABLE_PKGCONFIG)
  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/package/unilink.pc.in
      ${CMAKE_CURRENT_BINARY_DIR}/unilink.pc
      @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unilink.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# Examples
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Testing
include(CTest)
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

# CPack configuration for release packages
set(CPACK_PACKAGE_NAME "unilink")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Unified async communication library")
set(CPACK_PACKAGE_VENDOR "Jinwoo Sung")
set(CPACK_PACKAGE_CONTACT "jwsung91@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Set package generators
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

# Set package file name
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x64")

include(CPack)
