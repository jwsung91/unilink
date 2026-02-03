cmake_minimum_required(VERSION 3.12)

# Python bindings option
# Must be defined before project() so VCPKG_MANIFEST_FEATURES can be set correctly
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

# Enable vcpkg "python" feature if Python bindings are requested
# This must be set before project() where the toolchain is loaded
if(BUILD_PYTHON_BINDINGS)
  set(VCPKG_MANIFEST_FEATURES "python")
endif()

# Force vcpkg manifest mode and installation
# This ensures that vcpkg installs dependencies defined in vcpkg.json
# even if the toolchain detection is flaky or defaults are different.
set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_MANIFEST_INSTALL ON)

project(
  unilink
  VERSION 0.3.3
  DESCRIPTION
    "A cross-platform asynchronous C++ communication library providing a unified API for Serial, TCP, and UDP transports"
  HOMEPAGE_URL "https://github.com/jwsung91/unilink"
  LANGUAGES CXX
)

# Include build options
include(${CMAKE_CURRENT_LIST_DIR}/cmake/UnilinkOptions.cmake)

# Include compiler configuration (must remain before any target definitions)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/UnilinkCompiler.cmake)

# Include dependencies
include(${CMAKE_CURRENT_LIST_DIR}/cmake/UnilinkDependencies.cmake)

# Source files (explicitly listed, no globbing)
set(UNILINK_SOURCES
  unilink/transport/tcp_server/tcp_server.cc
  unilink/transport/tcp_client/tcp_client.cc
  unilink/transport/serial/serial.cc
  unilink/builder/auto_initializer.cc
  unilink/builder/serial_builder.cc
  unilink/builder/tcp_client_builder.cc
  unilink/builder/tcp_server_builder.cc
  unilink/builder/udp_builder.cc
  unilink/builder/unified_builder.cc
  unilink/concurrency/io_context_manager.cc
  unilink/config/config_factory.cc
  unilink/config/config_manager.cc
  unilink/diagnostics/error_handler.cc
  unilink/diagnostics/log_rotation.cc
  unilink/diagnostics/logger.cc
  unilink/factory/channel_factory.cc
  unilink/memory/memory_pool.cc
  unilink/memory/memory_tracker.cc
  unilink/memory/safe_data_buffer.cc
  unilink/transport/tcp_server/boost_tcp_acceptor.cc
  unilink/transport/tcp_server/boost_tcp_socket.cc
  unilink/transport/tcp_server/tcp_server_session.cc
  unilink/transport/udp/udp.cc
  unilink/util/input_validator.cc
  unilink/wrapper/serial/serial.cc
  unilink/wrapper/tcp_client/tcp_client.cc
  unilink/wrapper/tcp_server/tcp_server.cc
  unilink/wrapper/udp/udp.cc
)

# Public headers (explicitly listed, no globbing)
set(UNILINK_HEADERS
  unilink/base/common.hpp
  unilink/base/constants.hpp
  unilink/base/platform.hpp
  unilink/base/visibility.hpp
  unilink/builder/auto_initializer.hpp
  unilink/builder/ibuilder.hpp
  unilink/builder/serial_builder.hpp
  unilink/builder/tcp_client_builder.hpp
  unilink/builder/tcp_server_builder.hpp
  unilink/builder/udp_builder.hpp
  unilink/builder/unified_builder.hpp
  unilink/concurrency/io_context_manager.hpp
  unilink/concurrency/thread_safe_state.hpp
  unilink/config/config_factory.hpp
  unilink/config/config_manager.hpp
  unilink/config/iconfig_manager.hpp
  unilink/config/serial_config.hpp
  unilink/config/tcp_client_config.hpp
  unilink/config/tcp_server_config.hpp
  unilink/config/udp_config.hpp
  unilink/diagnostics/error_handler.hpp
  unilink/diagnostics/error_types.hpp
  unilink/diagnostics/exceptions.hpp
  unilink/diagnostics/log_rotation.hpp
  unilink/diagnostics/logger.hpp
  unilink/factory/channel_factory.hpp
  unilink/interface/channel.hpp
  unilink/interface/iserial_port.hpp
  unilink/interface/itcp_acceptor.hpp
  unilink/interface/itcp_resolver.hpp
  unilink/interface/itcp_socket.hpp
  unilink/interface/itimer.hpp
  unilink/memory/memory_pool.hpp
  unilink/memory/memory_tracker.hpp
  unilink/memory/memory_validator.hpp
  unilink/memory/safe_data_buffer.hpp
  unilink/memory/safe_span.hpp
  unilink/transport/serial/boost_serial_port.hpp
  unilink/transport/serial/serial.hpp
  unilink/transport/tcp_client/tcp_client.hpp
  unilink/transport/tcp_server/boost_tcp_acceptor.hpp
  unilink/transport/tcp_server/boost_tcp_socket.hpp
  unilink/transport/tcp_server/tcp_server_session.hpp
  unilink/transport/tcp_server/tcp_server.hpp
  unilink/transport/udp/udp.hpp
  unilink/unilink.hpp
  unilink/util/input_validator.hpp
  unilink/wrapper/ichannel.hpp
  unilink/wrapper/serial/serial.hpp
  unilink/wrapper/tcp_client/tcp_client.hpp
  unilink/wrapper/tcp_server/tcp_server.hpp
  unilink/wrapper/udp/udp.hpp
)

# Create the library
if(UNILINK_BUILD_SHARED AND UNILINK_BUILD_STATIC)
  # Build both shared and static libraries
  add_library(unilink_shared SHARED ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  add_library(unilink_static STATIC ${UNILINK_SOURCES} ${UNILINK_HEADERS})

  # Set target properties for shared library
  set_target_properties(
    unilink_shared
    PROPERTIES VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
               OUTPUT_NAME "unilink"
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
               WINDOWS_EXPORT_ALL_SYMBOLS OFF
  )

  # Set target properties for static library
  set_target_properties(
    unilink_static
    PROPERTIES OUTPUT_NAME "unilink_static"
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
  )

  target_compile_definitions(
    unilink_shared
    PUBLIC UNILINK_BUILD_SHARED
    PRIVATE UNILINK_BUILDING_LIBRARY
  )
  target_compile_definitions(unilink_static PUBLIC UNILINK_BUILD_STATIC)

  # Create unified target
  add_library(unilink INTERFACE)
  target_link_libraries(
    unilink
    INTERFACE $<$<TARGET_EXISTS:unilink_shared>:unilink_shared>
              $<$<NOT:$<TARGET_EXISTS:unilink_shared>>:unilink_static>
              unilink_dependencies
  )

elseif(UNILINK_BUILD_SHARED)
  # Build only shared library
  add_library(unilink SHARED ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  set_target_properties(
    unilink
    PROPERTIES VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR}
               OUTPUT_NAME "unilink"
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
               WINDOWS_EXPORT_ALL_SYMBOLS OFF
  )
  target_compile_definitions(
    unilink
    PUBLIC UNILINK_BUILD_SHARED
    PRIVATE UNILINK_BUILDING_LIBRARY
  )

elseif(UNILINK_BUILD_STATIC)
  # Build only static library
  add_library(unilink STATIC ${UNILINK_SOURCES} ${UNILINK_HEADERS})
  set_target_properties(
    unilink
    PROPERTIES OUTPUT_NAME "unilink"
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN YES
  )
  target_compile_definitions(unilink PUBLIC UNILINK_BUILD_STATIC)

else()
  message(
    FATAL_ERROR "At least one library type (shared or static) must be enabled"
  )
endif()

# Create aliases for both shared and static libraries
if(TARGET unilink_shared)
  add_library(unilink::unilink_shared ALIAS unilink_shared)
endif()
if(TARGET unilink_static)
  add_library(unilink::unilink_static ALIAS unilink_static)
endif()
if(TARGET unilink)
  add_library(unilink::unilink ALIAS unilink)
endif()

# Helper to stage runtime dependencies (primarily for Windows shared builds)
if(TARGET unilink_shared)
  set(UNILINK_SHARED_LIBRARY_TARGET unilink_shared)
elseif(UNILINK_BUILD_SHARED AND TARGET unilink)
  set(UNILINK_SHARED_LIBRARY_TARGET unilink)
endif()

function(unilink_configure_executable target)
  if(WIN32
     AND UNILINK_BUILD_SHARED
     AND UNILINK_SHARED_LIBRARY_TARGET
  )
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${UNILINK_SHARED_LIBRARY_TARGET}>
        $<TARGET_FILE_DIR:${target}>
      COMMENT
        "Copying ${UNILINK_SHARED_LIBRARY_TARGET} runtime to ${target} output directory"
    )
  endif()
endfunction()

# Link dependencies and set include directories
if(TARGET unilink_shared)
  target_link_libraries(unilink_shared PUBLIC unilink_dependencies)
  target_compile_features(unilink_shared PUBLIC cxx_std_17)
  target_include_directories(
    unilink_shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:include>
  )
endif()
if(TARGET unilink_static)
  target_link_libraries(unilink_static PUBLIC unilink_dependencies)
  target_compile_features(unilink_static PUBLIC cxx_std_17)
  target_include_directories(
    unilink_static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                          $<INSTALL_INTERFACE:include>
  )
endif()
if(TARGET unilink
   AND NOT TARGET unilink_shared
   AND NOT TARGET unilink_static
)
  target_link_libraries(unilink PUBLIC unilink_dependencies)
  target_compile_features(unilink PUBLIC cxx_std_17)
  target_include_directories(
    unilink PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                   $<INSTALL_INTERFACE:include>
  )
endif()

# Disable MSVC LTCG for unilink libraries to avoid link.exe access violations
# Disable MSVC LTCG for unilink libraries to avoid link.exe access violations.
# This is a known workaround. Future investigation should aim to resolve the
# underlying conflict that causes these violations when LTCG is enabled.
if(MSVC)
  set(_unilink_library_targets)
  if(TARGET unilink_shared)
    list(APPEND _unilink_library_targets unilink_shared)
  endif()
  if(TARGET unilink_static)
    list(APPEND _unilink_library_targets unilink_static)
  endif()
  if(TARGET unilink AND NOT (TARGET unilink_shared OR TARGET unilink_static))
    list(APPEND _unilink_library_targets unilink)
  endif()

  foreach(_target IN LISTS _unilink_library_targets)
    target_compile_options(
      ${_target}
      PRIVATE "$<$<CONFIG:Release>:/GL->" "$<$<CONFIG:RelWithDebInfo>:/GL->"
              "$<$<CONFIG:MinSizeRel>:/GL->"
    )
    set_property(
      TARGET ${_target}
      APPEND_STRING
      PROPERTY LINK_FLAGS_RELEASE " /LTCG:OFF"
    )
    set_property(
      TARGET ${_target}
      APPEND_STRING
      PROPERTY LINK_FLAGS_RELWITHDEBINFO " /LTCG:OFF"
    )
    set_property(
      TARGET ${_target}
      APPEND_STRING
      PROPERTY LINK_FLAGS_MINSIZEREL " /LTCG:OFF"
    )
  endforeach()
endif()

# Export header generation
if(UNILINK_ENABLE_EXPORT_HEADER)
  include(GenerateExportHeader)

  # Generate export header for the main target
  if(TARGET unilink_shared)
    generate_export_header(
      unilink_shared EXPORT_FILE_NAME
      "${CMAKE_CURRENT_BINARY_DIR}/unilink_export.hpp" BASE_NAME UNILINK
    )
    target_include_directories(
      unilink_shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
  elseif(TARGET unilink)
    generate_export_header(
      unilink EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/unilink_export.hpp"
      BASE_NAME UNILINK
    )
    target_include_directories(
      unilink PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
  endif()
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

if(BUILD_PYTHON_BINDINGS)
  add_subdirectory(bindings/python)
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
  add_custom_target(
    docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )

  message(STATUS "Documentation target 'docs' available")
else()
  # Create a dummy docs target to prevent build failures
  add_custom_target(
    docs
    COMMAND
      echo
      "Documentation generation skipped - Doxygen not available or disabled"
    COMMENT "Documentation generation skipped"
  )
endif()

# Include packaging configuration
include(cmake/UnilinkPackaging.cmake)