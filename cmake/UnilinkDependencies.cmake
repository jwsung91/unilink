# Unilink dependencies management
# This file handles all external dependencies

# Keep FindBoost module available (CMP0167 makes it opt-in/removed in newer CMake)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31" AND POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

# Normalize Boost lookup variants to avoid missing component builds
set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Prefer shared Boost libraries" FORCE)
set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multithreaded Boost libraries" FORCE)
set(Boost_USE_DEBUG_RUNTIME OFF CACHE BOOL "Do not require debug runtime Boost binaries" FORCE)
set(UNILINK_BOOST_COMPONENTS system)

# Control whether we must link Boost.System (off on macOS to use header-only mode)
set(UNILINK_LINK_BOOST_SYSTEM ON)

# Check if we should enforce system/vcpkg boost or allow fallback
set(_boost_req REQUIRED)
if(NOT DEFINED VCPKG_TARGET_TRIPLET AND NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
  set(_boost_req QUIET)
endif()

# Homebrew's BoostConfig packages on macOS (e.g., Boost 1.89) often miss
# per-component config files such as boost_system-config.cmake, which causes
# configure-time failures when CMake picks the config package first. Prefer the
# classic FindBoost module on macOS unless the user explicitly overrides the
# behavior.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT DEFINED Boost_NO_BOOST_CMAKE)
  set(Boost_NO_BOOST_CMAKE ON CACHE BOOL
      "Prefer FindBoost module over BoostConfig on macOS to avoid missing component configs")
  message(STATUS "Forcing FindBoost module on macOS to avoid missing Boost component config files")
endif()

# Find required packages
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # Windows builds: rely on recent Boost releases available via Conan/vcpkg
  find_package(Boost 1.70 ${_boost_req} COMPONENTS ${UNILINK_BOOST_COMPONENTS})
  if(Boost_FOUND)
    message(STATUS "Using Boost 1.70+ for Windows compatibility")
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Detect Ubuntu version and set appropriate Boost version
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0")
    # Ubuntu 20.04: GCC 9-10, Boost 1.65+
    find_package(Boost 1.65 ${_boost_req} COMPONENTS ${UNILINK_BOOST_COMPONENTS})
    if(Boost_FOUND)
      message(STATUS "Using Boost 1.65+ for Ubuntu 20.04 compatibility")
    endif()
  elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
    # Ubuntu 22.04: GCC 11-12, Boost 1.74+
    find_package(Boost 1.74 ${_boost_req} COMPONENTS ${UNILINK_BOOST_COMPONENTS})
    if(Boost_FOUND)
      message(STATUS "Using Boost 1.74+ for Ubuntu 22.04 compatibility")
    endif()
  else()
    # Ubuntu 24.04+: GCC 13+, try Boost 1.83+ first, fallback to 1.74+
    find_package(Boost 1.83 QUIET COMPONENTS ${UNILINK_BOOST_COMPONENTS})
    if(Boost_FOUND)
      message(STATUS "Using Boost 1.83+ for Ubuntu 24.04+ compatibility")
    else()
      find_package(Boost 1.74 ${_boost_req} COMPONENTS ${UNILINK_BOOST_COMPONENTS})
      if(Boost_FOUND)
        message(STATUS "Using Boost 1.74+ for Ubuntu 24.04+ compatibility (1.83 not available)")
      endif()
    endif()
  endif()
  if(NOT Boost_FOUND)
    # Manual fallback if the packaged Boost variants don't match expectations (e.g., debug runtime mismatch)
    find_path(BOOST_FALLBACK_INCLUDE_DIR boost/version.hpp
      HINTS /usr/include /usr/local/include)
    find_library(BOOST_FALLBACK_SYSTEM_LIB NAMES boost_system
      HINTS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib)
    if(BOOST_FALLBACK_INCLUDE_DIR AND BOOST_FALLBACK_SYSTEM_LIB AND EXISTS "${BOOST_FALLBACK_INCLUDE_DIR}/boost/asio.hpp")
      if(NOT TARGET Boost::system)
        add_library(Boost::system UNKNOWN IMPORTED)
        set_target_properties(Boost::system PROPERTIES
          IMPORTED_LOCATION "${BOOST_FALLBACK_SYSTEM_LIB}"
          INTERFACE_INCLUDE_DIRECTORIES "${BOOST_FALLBACK_INCLUDE_DIR}")
      endif()
      set(Boost_FOUND TRUE)
      message(STATUS "Using manual Boost::system fallback on Linux at ${BOOST_FALLBACK_SYSTEM_LIB}")
    endif()
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # vcpkg builds: let the vcpkg toolchain drive Boost discovery (do not force Homebrew paths)
  if(DEFINED VCPKG_TARGET_TRIPLET OR DEFINED ENV{VCPKG_ROOT} OR CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
    find_package(Boost 1.70 ${_boost_req} COMPONENTS ${UNILINK_BOOST_COMPONENTS})
    set(UNILINK_LINK_BOOST_SYSTEM ON)
    if(Boost_FOUND)
      message(STATUS "Using Boost from vcpkg on macOS (triplet: ${VCPKG_TARGET_TRIPLET})")
    endif()
  else()
    # Ensure Boost module lookup (skip BoostConfig.cmake) and point at Homebrew layout
    if(POLICY CMP0144)
      cmake_policy(SET CMP0144 NEW)
    endif()
    set(Boost_NO_BOOST_CMAKE ON CACHE BOOL "" FORCE)
    set(Boost_DIR "" CACHE PATH "Ignore Boost config packages on macOS" FORCE)
    set(BOOST_ROOT "/opt/homebrew/opt/boost" CACHE PATH "Homebrew Boost root" FORCE)
    set(BOOST_INCLUDEDIR "/opt/homebrew/opt/boost/include" CACHE PATH "Homebrew Boost include dir" FORCE)
    set(Boost_ADDITIONAL_VERSIONS "1.89" "1.89.0" CACHE STRING "" FORCE)
    list(APPEND CMAKE_PREFIX_PATH
      /opt/homebrew/opt/boost
      /opt/homebrew
      /usr/local/opt/boost
      /usr/local)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_ROOT}/Modules")

    # Header-only Boost.System: only require headers, avoid lib lookup that is flaky on macOS Actions
    find_path(BOOST_FALLBACK_INCLUDE_DIR boost/version.hpp
      PATHS
        /opt/homebrew/opt/boost
        /opt/homebrew
        /usr/local/opt/boost
        /usr/local
        /opt/homebrew/Cellar/boost
        /usr/local/Cellar/boost
      PATH_SUFFIXES include)
    if(NOT BOOST_FALLBACK_INCLUDE_DIR OR NOT EXISTS "${BOOST_FALLBACK_INCLUDE_DIR}/boost/asio.hpp")
      if("${_boost_req}" STREQUAL "REQUIRED")
        message(FATAL_ERROR "Boost headers not found on macOS (looked in Homebrew paths)")
      endif()
    else()
      set(UNILINK_LINK_BOOST_SYSTEM OFF)
      set(UNILINK_BOOST_INCLUDE_DIR "${BOOST_FALLBACK_INCLUDE_DIR}")
      add_compile_definitions(BOOST_ERROR_CODE_HEADER_ONLY BOOST_SYSTEM_NO_LIB)
      set(Boost_FOUND TRUE)
      message(STATUS "Using header-only Boost.System on macOS; include dir: ${BOOST_FALLBACK_INCLUDE_DIR}")
    endif()
  endif()
else()
  # Other platforms: use a recent Boost version
  find_package(Boost 1.70 ${_boost_req} COMPONENTS ${UNILINK_BOOST_COMPONENTS})
endif()

# Fallback using FetchContent
if(NOT Boost_FOUND AND "${_boost_req}" STREQUAL "QUIET")
  message(STATUS "Boost not found. Attempting to fetch Boost via FetchContent...")
  include(FetchContent)

  FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )

  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_INCLUDE_LIBRARIES system asio)

  FetchContent_MakeAvailable(Boost)

  if(TARGET Boost::system)
     set(Boost_FOUND TRUE)
     set(UNILINK_LINK_BOOST_SYSTEM ON)
     set(UNILINK_BOOST_INCLUDE_DIR ${boost_SOURCE_DIR})
     message(STATUS "Boost fetched and built successfully via FetchContent.")
  else()
     # If Boost::system target is missing, fallback to headers + header-only libs
     # Note: boost_SOURCE_DIR is set by FetchContent_MakeAvailable
     set(Boost_FOUND TRUE)
     set(UNILINK_LINK_BOOST_SYSTEM OFF)
     set(UNILINK_BOOST_INCLUDE_DIR ${boost_SOURCE_DIR})
     add_compile_definitions(BOOST_ERROR_CODE_HEADER_ONLY BOOST_SYSTEM_NO_LIB)
     message(STATUS "Boost::system target not created. Using header-only mode from fetched source.")
  endif()
endif()

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found. Please install Boost or use vcpkg.")
endif()

# Ensure Boost.Asio headers are present even when BoostConfig packages omit an asio component
if(NOT UNILINK_BOOST_INCLUDE_DIR)
  set(_boost_asio_search_paths
    ${Boost_INCLUDE_DIRS}
    ${BOOST_INCLUDEDIR}
    ${BOOST_ROOT}
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    /opt/homebrew/opt/boost/include
    /opt/homebrew/Cellar/boost/*/include)
  find_path(BOOST_ASIO_HEADER boost/asio.hpp
    PATHS ${_boost_asio_search_paths}
    PATH_SUFFIXES include
    NO_CACHE)
  if(NOT BOOST_ASIO_HEADER)
    message(FATAL_ERROR "Boost.Asio headers not found. Install boost-asio/boost headers or set BOOST_ROOT.")
  endif()
endif()

find_package(Threads REQUIRED)

# Optional dependencies
find_package(PkgConfig QUIET)

# Google Test for testing
# Note: Guarded so packaging environments (e.g., vcpkg with FETCHCONTENT_FULLY_DISCONNECTED)
# can disable all tests and avoid downloading GoogleTest.
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

  # Silence noisy sign-conversion warnings inside GoogleTest sources on GCC/Clang
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(gtest PRIVATE -Wno-sign-conversion -Wno-conversion)
    target_compile_options(gmock PRIVATE -Wno-sign-conversion -Wno-conversion)
    target_compile_options(gtest_main PRIVATE -Wno-sign-conversion -Wno-conversion)
    target_compile_options(gmock_main PRIVATE -Wno-sign-conversion -Wno-conversion)
  endif()
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
  Threads::Threads
)
if(UNILINK_LINK_BOOST_SYSTEM)
  target_link_libraries(unilink_dependencies INTERFACE Boost::system)
  if(TARGET Boost::asio)
    target_link_libraries(unilink_dependencies INTERFACE Boost::asio)
  endif()
elseif(UNILINK_BOOST_INCLUDE_DIR)
  target_include_directories(unilink_dependencies INTERFACE "${UNILINK_BOOST_INCLUDE_DIR}")
endif()
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
  if(APPLE)
    # macOS needs BSD extensions for networking macros (NI_MAXHOST, SO_NOSIGPIPE, etc.)
    target_compile_definitions(unilink_dependencies INTERFACE
      _DARWIN_C_SOURCE
    )
  else()
    target_compile_definitions(unilink_dependencies INTERFACE
      _POSIX_C_SOURCE=200809L
    )
  endif()
endif()

# Feature flags
if(UNILINK_ENABLE_CONFIG)
  target_compile_definitions(unilink_dependencies INTERFACE UNILINK_ENABLE_CONFIG=1)
endif()

if(UNILINK_ENABLE_MEMORY_TRACKING)
  target_compile_definitions(unilink_dependencies INTERFACE UNILINK_ENABLE_MEMORY_TRACKING=1)
endif()

# Export dependencies for downstream projects
set(_UNILINK_DEPENDENCY_TARGETS Threads::Threads)
if(UNILINK_LINK_BOOST_SYSTEM)
  list(APPEND _UNILINK_DEPENDENCY_TARGETS Boost::system)
endif()
set(UNILINK_DEPENDENCIES
  ${_UNILINK_DEPENDENCY_TARGETS}
  CACHE INTERNAL "Unilink dependencies")
