# Unilink dependencies management. This file handles all external dependencies.

# Keep FindBoost module available (CMP0167 makes it opt-in/removed in newer
# CMake)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31" AND POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

set(UNILINK_MIN_BOOST_VERSION
    1.84.0
    CACHE STRING "Minimum supported Boost version"
)

# Normalize Boost lookup variants to avoid missing component builds
if(WIN32 OR (DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES
                                              "static")
)
  set(Boost_USE_STATIC_LIBS
      ON
      CACHE BOOL "Use static Boost libraries" FORCE
  )
else()
  set(Boost_USE_STATIC_LIBS
      OFF
      CACHE BOOL "Prefer shared Boost libraries" FORCE
  )
endif()
set(Boost_USE_MULTITHREADED
    ON
    CACHE BOOL "Use multithreaded Boost libraries" FORCE
)
set(Boost_USE_DEBUG_RUNTIME
    OFF
    CACHE BOOL "Do not require debug runtime Boost binaries" FORCE
)
set(UNILINK_BOOST_COMPONENTS system)

set(UNILINK_LINK_BOOST_SYSTEM ON)

# Homebrew's BoostConfig packages on macOS can miss per-component config files.
# Prefer FindBoost for non-vcpkg macOS builds unless explicitly overridden.
set(_unilink_using_vcpkg OFF)
if(DEFINED VCPKG_TARGET_TRIPLET OR CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg")
  set(_unilink_using_vcpkg ON)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin"
   AND NOT _unilink_using_vcpkg
   AND NOT DEFINED Boost_NO_BOOST_CMAKE
)
  set(Boost_NO_BOOST_CMAKE
      ON
      CACHE
        BOOL
        "Prefer FindBoost module over BoostConfig on macOS to avoid missing component configs"
  )
  message(
    STATUS
      "Forcing FindBoost module on macOS to avoid missing Boost component config files"
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT _unilink_using_vcpkg)
  if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
  endif()
  set(BOOST_ROOT
      "/opt/homebrew/opt/boost"
      CACHE PATH "Homebrew Boost root" FORCE
  )
  set(BOOST_INCLUDEDIR
      "/opt/homebrew/opt/boost/include"
      CACHE PATH "Homebrew Boost include dir" FORCE
  )
  set(Boost_ADDITIONAL_VERSIONS
      "1.89" "1.89.0" "1.84" "1.84.0"
      CACHE STRING "" FORCE
  )
  list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/boost /opt/homebrew
       /usr/local/opt/boost /usr/local
  )
endif()

find_package(
  Boost ${UNILINK_MIN_BOOST_VERSION} REQUIRED
  COMPONENTS ${UNILINK_BOOST_COMPONENTS}
)

if(_unilink_using_vcpkg)
  message(
    STATUS
      "Using Boost ${Boost_VERSION} from vcpkg (minimum ${UNILINK_MIN_BOOST_VERSION})"
  )
else()
  message(
    STATUS
      "Using Boost ${Boost_VERSION} from the configured package environment (minimum ${UNILINK_MIN_BOOST_VERSION})"
  )
endif()

# Ensure Boost.Asio headers are present even when BoostConfig packages omit an
# asio component
if(NOT UNILINK_BOOST_INCLUDE_DIR)
  set(_boost_asio_search_paths
      ${Boost_INCLUDE_DIRS}
      ${BOOST_INCLUDEDIR}
      ${BOOST_ROOT}
      /usr/include
      /usr/local/include
      /opt/homebrew/include
      /opt/homebrew/opt/boost/include
      /opt/homebrew/Cellar/boost/*/include
  )
  find_path(
    BOOST_ASIO_HEADER boost/asio.hpp
    PATHS ${_boost_asio_search_paths}
    PATH_SUFFIXES include NO_CACHE
  )
  if(NOT BOOST_ASIO_HEADER)
    message(
      FATAL_ERROR
        "Boost.Asio headers not found. Install boost-asio/boost headers or set BOOST_ROOT."
    )
  endif()
endif()

find_package(spdlog 1.9 CONFIG QUIET)
if(NOT spdlog_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
  )
  FetchContent_MakeAvailable(spdlog)
endif()
find_package(Threads REQUIRED)

# Optional dependencies
find_package(PkgConfig QUIET)

# Google Test for testing Note: Guarded so packaging environments (e.g., vcpkg
# with FETCHCONTENT_FULLY_DISCONNECTED) can disable all tests and avoid
# downloading GoogleTest.
if(UNILINK_BUILD_TESTS)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
  )

  # Prevent GoogleTest from overriding our compiler/linker options
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE
  )
  set(INSTALL_GTEST
      OFF
      CACHE BOOL "" FORCE
  )
  set(BUILD_GMOCK
      ON
      CACHE BOOL "" FORCE
  )
  set(gtest_build_tests
      OFF
      CACHE BOOL "" FORCE
  )
  set(gtest_build_samples
      OFF
      CACHE BOOL "" FORCE
  )
  set(gtest_build_benchmarks
      OFF
      CACHE BOOL "" FORCE
  )

  FetchContent_MakeAvailable(googletest)

  # Create alias for easier usage
  add_library(GTest::gtest ALIAS gtest)
  add_library(GTest::gtest_main ALIAS gtest_main)
  add_library(GTest::gmock ALIAS gmock)
  add_library(GTest::gmock_main ALIAS gmock_main)

  # Silence noisy sign-conversion warnings inside GoogleTest sources on
  # GCC/Clang
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(gtest PRIVATE -Wno-sign-conversion -Wno-conversion)
    target_compile_options(gmock PRIVATE -Wno-sign-conversion -Wno-conversion)
    target_compile_options(
      gtest_main PRIVATE -Wno-sign-conversion -Wno-conversion
    )
    target_compile_options(
      gmock_main PRIVATE -Wno-sign-conversion -Wno-conversion
    )
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
target_link_libraries(unilink_dependencies INTERFACE Threads::Threads)
if(UNILINK_LINK_BOOST_SYSTEM)
  if(UNILINK_BOOST_FETCHED)
    # For local targets (FetchContent), use BUILD_INTERFACE to avoid export
    # errors and INSTALL_INTERFACE with a proxy target to avoid conflicts with
    # the local target
    target_link_libraries(
      unilink_dependencies
      INTERFACE $<BUILD_INTERFACE:Boost::system>
                $<INSTALL_INTERFACE:unilink_boost_system_proxy>
    )
    if(TARGET Boost::asio)
      target_link_libraries(
        unilink_dependencies
        INTERFACE $<BUILD_INTERFACE:Boost::asio>
                  $<INSTALL_INTERFACE:unilink_boost_asio_proxy>
      )
    endif()
  else()
    target_link_libraries(unilink_dependencies INTERFACE Boost::system)
    if(TARGET Boost::asio)
      target_link_libraries(unilink_dependencies INTERFACE Boost::asio)
    endif()
  endif()
endif()

if(UNILINK_BOOST_INCLUDE_DIR)
  target_include_directories(
    unilink_dependencies SYSTEM
    INTERFACE $<BUILD_INTERFACE:${UNILINK_BOOST_INCLUDE_DIR}>
  )
elseif(UNILINK_BOOST_INCLUDE_DIR)
  target_include_directories(
    unilink_dependencies INTERFACE "${UNILINK_BOOST_INCLUDE_DIR}"
  )
endif()
if(WIN32)
  target_link_libraries(unilink_dependencies INTERFACE ws2_32 mswsock iphlpapi)
endif()

# Add include directories
target_include_directories(
  unilink_dependencies INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                                 $<INSTALL_INTERFACE:include>
)

# Add compile definitions
target_compile_definitions(
  unilink_dependencies INTERFACE $<$<CONFIG:Debug>:UNILINK_DEBUG=1>
                                 $<$<CONFIG:Release>:UNILINK_RELEASE=1>
)

# Platform-specific definitions
if(WIN32)
  target_compile_definitions(
    unilink_dependencies INTERFACE WIN32_LEAN_AND_MEAN NOMINMAX
  )
elseif(UNIX)
  if(APPLE)
    # macOS needs BSD extensions for networking macros (NI_MAXHOST,
    # SO_NOSIGPIPE, etc.)
    target_compile_definitions(unilink_dependencies INTERFACE _DARWIN_C_SOURCE)
  else()
    target_compile_definitions(
      unilink_dependencies INTERFACE _POSIX_C_SOURCE=200809L
    )
  endif()
endif()

# Feature flags
if(UNILINK_ENABLE_CONFIG)
  target_compile_definitions(
    unilink_dependencies INTERFACE UNILINK_ENABLE_CONFIG=1
  )
endif()

if(UNILINK_ENABLE_MEMORY_TRACKING)
  target_compile_definitions(
    unilink_dependencies INTERFACE UNILINK_ENABLE_MEMORY_TRACKING=1
  )
endif()

# Export dependencies for downstream projects
set(_UNILINK_DEPENDENCY_TARGETS Threads::Threads)
if(UNILINK_LINK_BOOST_SYSTEM)
  list(APPEND _UNILINK_DEPENDENCY_TARGETS Boost::system)
endif()
set(UNILINK_DEPENDENCIES
    ${_UNILINK_DEPENDENCY_TARGETS}
    CACHE INTERNAL "Unilink dependencies"
)
