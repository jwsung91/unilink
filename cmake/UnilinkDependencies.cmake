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
  # Ensure Boost module lookup (skip BoostConfig.cmake) and point at Homebrew layout
  if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
  endif()
  set(Boost_NO_BOOST_CMAKE ON CACHE BOOL "" FORCE)
  set(Boost_DIR "" CACHE PATH "Ignore Boost config packages on macOS" FORCE)
  set(BOOST_ROOT "/opt/homebrew" CACHE PATH "Homebrew root" FORCE)
  set(BOOST_INCLUDEDIR "/opt/homebrew/include" CACHE PATH "Homebrew Boost include dir" FORCE)
  set(BOOST_LIBRARYDIR "/opt/homebrew/lib" CACHE PATH "Homebrew Boost library dir" FORCE)
  list(APPEND CMAKE_PREFIX_PATH /opt/homebrew /opt/homebrew/opt/boost)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_ROOT}/Modules")
  find_package(Boost 1.74 MODULE REQUIRED COMPONENTS system)
  message(STATUS "Using Boost 1.74+ for macOS compatibility")
else()
  # Other platforms: use a recent Boost version
  find_package(Boost 1.70 REQUIRED COMPONENTS system)
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
