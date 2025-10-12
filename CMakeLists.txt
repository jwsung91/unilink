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
  DESCRIPTION "Unified async communication library"
  HOMEPAGE_URL "https://github.com/jwsung91/unilink"
  LANGUAGES CXX)

# Include build options
include(cmake/UnilinkOptions.cmake)

# Include compiler configuration
include(cmake/UnilinkCompiler.cmake)

# Include dependencies
include(cmake/UnilinkDependencies.cmake)

# Source files
file(GLOB_RECURSE UNILINK_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/**/*.cc"
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/*.cc")

# Public headers (for CMake target)
file(GLOB_RECURSE UNILINK_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/**/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/unilink/**/*.h")

# Create the library
if(UNILINK_BUILD_SHARED AND UNILINK_BUILD_STATIC)
  # Build both shared and static libraries
  add_library(unilink_shared SHARED ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  add_library(unilink_static STATIC ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  
  # Set target properties for shared library
  set_target_properties(unilink_shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "unilink"
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN NO
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
  
  # Set target properties for static library
  set_target_properties(unilink_static PROPERTIES
    OUTPUT_NAME "unilink_static"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES)
  
  # Create unified target
  add_library(unilink INTERFACE)
  target_link_libraries(unilink INTERFACE
    $<$<TARGET_EXISTS:unilink_shared>:unilink_shared>
    $<$<NOT:$<TARGET_EXISTS:unilink_shared>>:unilink_static>
    unilink_dependencies)
    
elseif(UNILINK_BUILD_SHARED)
  # Build only shared library
  add_library(unilink SHARED ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  set_target_properties(unilink PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "unilink"
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN NO
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
    
elseif(UNILINK_BUILD_STATIC)
  # Build only static library
  add_library(unilink STATIC ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  set_target_properties(unilink PROPERTIES
    OUTPUT_NAME "unilink"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES)
    
else()
  message(FATAL_ERROR "At least one library type (shared or static) must be enabled")
endif()

# Create aliases for both shared and static libraries
if(TARGET unilink_shared)
  add_library(unilink::unilink_shared ALIAS unilink_shared)
endif()
if(TARGET unilink_static)
  add_library(unilink::unilink_static ALIAS unilink_static)
endif()
if(TARGET unilink AND NOT unilink_shared AND NOT unilink_static)
  add_library(unilink::unilink ALIAS unilink)
endif()

# Link dependencies and set include directories
if(TARGET unilink_shared)
  target_link_libraries(unilink_shared PUBLIC unilink_dependencies)
  target_compile_features(unilink_shared PUBLIC cxx_std_17)
  target_include_directories(unilink_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
endif()
if(TARGET unilink_static)
  target_link_libraries(unilink_static PUBLIC unilink_dependencies)
  target_compile_features(unilink_static PUBLIC cxx_std_17)
  target_include_directories(unilink_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
endif()
if(TARGET unilink AND NOT unilink_shared AND NOT unilink_static)
  target_link_libraries(unilink INTERFACE unilink_dependencies)
  target_compile_features(unilink INTERFACE cxx_std_17)
  target_include_directories(unilink INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
endif()

# Export header generation
if(UNILINK_ENABLE_EXPORT_HEADER)
  include(GenerateExportHeader)
  
  # Generate export header for the main target
  if(TARGET unilink_shared)
    generate_export_header(unilink_shared
      EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/unilink_export.hpp"
      BASE_NAME UNILINK)
    target_include_directories(unilink_shared PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
  elseif(TARGET unilink)
    generate_export_header(unilink
      EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/unilink_export.hpp"
      BASE_NAME UNILINK)
    target_include_directories(unilink PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
  endif()
endif()

# Install configuration
include(GNUInstallDirs)
if(UNILINK_ENABLE_INSTALL)
  # Install targets
  if(TARGET unilink_shared)
    install(TARGETS unilink_shared
      EXPORT unilinkTargets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()
  
  if(TARGET unilink_static)
    install(TARGETS unilink_static
      EXPORT unilinkTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  endif()
  
  # unilink::unilink is an alias, so it doesn't need to be exported
  # Consumers should use unilink_shared or unilink_static directly

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

  # Install dependencies target
  install(TARGETS unilink_dependencies
      EXPORT unilinkTargets
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unilink)
  
  install(EXPORT unilinkTargets
      NAMESPACE unilink::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/unilink)
endif()

# pkg-config support
if(UNILINK_ENABLE_INSTALL AND UNILINK_ENABLE_PKGCONFIG)
  # Set pkg-config variables
  set(PKGCONFIG_REQUIRES "")
  set(PKGCONFIG_REQUIRES_PRIVATE "")
  set(PKGCONFIG_LIBS_PRIVATE "-lboost_system -lpthread")
  set(PKGCONFIG_CFLAGS "")
  
  configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/package/unilink.pc.in
      ${CMAKE_CURRENT_BINARY_DIR}/unilink.pc
      @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unilink.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# Examples
if(UNILINK_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Testing
if(UNILINK_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

# Documentation generation with Doxygen
if(UNILINK_BUILD_DOCS AND UNILINK_DOXYGEN_AVAILABLE)
  # Create docs directory
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/config)
  
  # Generate Doxyfile if it doesn't exist
  set(DOXYFILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/docs/config/Doxyfile)
  if(NOT EXISTS ${DOXYFILE_PATH})
    message(STATUS "Generating Doxyfile...")
    execute_process(
      COMMAND ${DOXYGEN_EXECUTABLE} -g ${DOXYFILE_PATH}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/config
      RESULT_VARIABLE DOXYGEN_GEN_RESULT
    )
    if(NOT DOXYGEN_GEN_RESULT EQUAL 0)
      message(WARNING "Failed to generate Doxyfile")
    endif()
  endif()
  
  # Add custom target for documentation
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
  
  # Add install target for documentation
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/html/
    DESTINATION share/doc/unilink/html
    OPTIONAL
  )
  
  message(STATUS "Documentation target 'docs' available")
else()
  # Create a dummy docs target to prevent build failures
  add_custom_target(docs
    COMMAND echo "Documentation generation skipped - Doxygen not available or disabled"
    COMMENT "Documentation generation skipped"
  )
endif()

# Include packaging configuration
include(cmake/UnilinkPackaging.cmake)
