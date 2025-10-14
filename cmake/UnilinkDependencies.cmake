# Unilink dependencies management
# This file handles all external dependencies

# Set CMake policy to suppress FindBoost deprecation warning
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
  cmake_policy(SET CMP0167 NEW)
endif()

# Find required packages
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Windows builds: rely on recent Boost releases available via Conan/vcpkg
  find_package(Boost 1.70 REQUIRED COMPONENTS system)
  message(STATUS "Using Boost 1.70+ for Windows compatibility")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Detect Ubuntu version and set appropriate Boost version
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0")
    # Ubuntu 20.04: GCC 9-10, Boost 1.65+
    find_package(Boost 1.65 REQUIRED COMPONENTS system)
    message(STATUS "Using Boost 1.65+ for Ubuntu 20.04 compatibility")
  elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
    # Ubuntu 22.04: GCC 11-12, Boost 1.74+
    find_package(Boost 1.74 REQUIRED COMPONENTS system)
    message(STATUS "Using Boost 1.74+ for Ubuntu 22.04 compatibility")
  else()
    # Ubuntu 24.04+: GCC 13+, try Boost 1.83+ first, fallback to 1.74+
    find_package(Boost 1.83 QUIET COMPONENTS system)
    if(Boost_FOUND)
      message(STATUS "Using Boost 1.83+ for Ubuntu 24.04+ compatibility")
    else()
      find_package(Boost 1.74 REQUIRED COMPONENTS system)
      message(STATUS "Using Boost 1.74+ for Ubuntu 24.04+ compatibility (1.83 not available)")
    endif()
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  find_package(Boost 1.74 REQUIRED COMPONENTS system)
  message(STATUS "Using Boost 1.74+ for macOS compatibility")
else()
  # Other platforms: use a recent Boost version
  find_package(Boost 1.70 REQUIRED COMPONENTS system)
endif()

find_package(Threads REQUIRED)

# Optional dependencies
find_package(PkgConfig QUIET)

# Google Test for testing
if(UNILINK_BUILD_TESTS)
  include(FetchContent)
  
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
  )
  
  # Prevent GoogleTest from overriding our compiler/linker options
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  set(gtest_build_tests OFF CACHE BOOL "" FORCE)
  set(gtest_build_samples OFF CACHE BOOL "" FORCE)
  set(gtest_build_benchmarks OFF CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(googletest)
  
  # Create alias for easier usage
  add_library(GTest::gtest ALIAS gtest)
  add_library(GTest::gtest_main ALIAS gtest_main)
  add_library(GTest::gmock ALIAS gmock)
  add_library(GTest::gmock_main ALIAS gmock_main)
endif()

# Doxygen for documentation
if(UNILINK_BUILD_DOCS)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    set(UNILINK_DOXYGEN_AVAILABLE ON)
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
  else()
    message(WARNING "Doxygen not found. Documentation will not be generated.")
    message(STATUS "Install Doxygen to enable documentation generation:")
    message(STATUS "  Ubuntu/Debian: sudo apt install doxygen")
    message(STATUS "  CentOS/RHEL: sudo yum install doxygen")
    message(STATUS "  macOS: brew install doxygen")
    message(STATUS "  Windows: choco install doxygen")
  endif()
endif()

# Create interface library for dependencies
add_library(unilink_dependencies INTERFACE)

# Link common dependencies
target_link_libraries(unilink_dependencies INTERFACE
  Boost::system
  Threads::Threads
)
if(WIN32)
  target_link_libraries(unilink_dependencies INTERFACE
    ws2_32
    mswsock
    iphlpapi
  )
endif()

# Add include directories
target_include_directories(unilink_dependencies INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

# Add compile definitions
target_compile_definitions(unilink_dependencies INTERFACE
  $<$<CONFIG:Debug>:UNILINK_DEBUG=1>
  $<$<CONFIG:Release>:UNILINK_RELEASE=1>
)

# Platform-specific definitions
if(WIN32)
  target_compile_definitions(unilink_dependencies INTERFACE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
  )
elseif(UNIX)
  target_compile_definitions(unilink_dependencies INTERFACE
    _POSIX_C_SOURCE=200809L
  )
endif()

# Feature flags
if(UNILINK_ENABLE_CONFIG)
  target_compile_definitions(unilink_dependencies INTERFACE UNILINK_ENABLE_CONFIG=1)
endif()

if(UNILINK_ENABLE_MEMORY_TRACKING)
  target_compile_definitions(unilink_dependencies INTERFACE UNILINK_ENABLE_MEMORY_TRACKING=1)
endif()

# Export dependencies for downstream projects
set(UNILINK_DEPENDENCIES
  Boost::system
  Threads::Threads
  CACHE INTERNAL "Unilink dependencies"
)
