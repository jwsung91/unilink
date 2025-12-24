# Unilink compiler configuration
# This file handles compiler-specific settings and optimizations.
#
# IMPORTANT:
#   Do NOT redefine compiler/toolchain-provided platform/architecture macros
#   such as _WIN32, _WIN64, _M_IX86, _M_X64, _M_AMD64, etc.
#   Those macros are consumed by the Windows SDK, STL and third-party libraries.

# -----------------------------------------------------------------------------
# Compiler-specific flags
# -----------------------------------------------------------------------------

if(MSVC)
  # Microsoft Visual C++ (including clang-cl)
  set(UNILINK_COMPILER_MSVC ON)

  # Remove any existing /W[0-4] flags the generator might have added so we
  # control warning level explicitly.
  foreach(flag_var
          CMAKE_C_FLAGS
          CMAKE_CXX_FLAGS
          CMAKE_C_FLAGS_DEBUG
          CMAKE_CXX_FLAGS_DEBUG
          CMAKE_C_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_RELEASE
          CMAKE_C_FLAGS_RELWITHDEBINFO
          CMAKE_CXX_FLAGS_RELWITHDEBINFO
          CMAKE_C_FLAGS_MINSIZEREL
          CMAKE_CXX_FLAGS_MINSIZEREL)
    if(DEFINED ${flag_var})
      string(REGEX REPLACE "(/W[0-4])" "" _cleaned_flags "${${flag_var}}")
      string(REGEX REPLACE "  +" " " _cleaned_flags "${_cleaned_flags}")
      string(STRIP "${_cleaned_flags}" _cleaned_flags)
      set(${flag_var} "${_cleaned_flags}" CACHE STRING "" FORCE)
    endif()
  endforeach()

  if(UNILINK_ENABLE_WARNINGS)
    add_compile_options(/W4 /permissive- /utf-8)
    if(UNILINK_ENABLE_WERROR)
      add_compile_options(/WX)
    endif()
  endif()

  # MSVC-specific optimizations and debug information
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /Oy /GL")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /O1 /Os")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # GCC or Clang
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(UNILINK_COMPILER_GCC ON)
  else()
    set(UNILINK_COMPILER_CLANG ON)
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    message(FATAL_ERROR "GCC 9.0+ or Clang 10.0+ required for C++17 support")
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_definitions(UNILINK_PLATFORM_MACOS=1)
    message(STATUS "Detected macOS compatibility mode")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(UNILINK_PLATFORM_LINUX=1)
    message(STATUS "Detected Linux compatibility mode")
  else()
    add_compile_definitions(UNILINK_PLATFORM_POSIX=1)
    message(STATUS "Detected generic POSIX compatibility mode")
  endif()

  # Common flags for GCC and Clang
  if(UNILINK_ENABLE_WARNINGS)
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)

    # Suppress warnings for older GCC versions (equivalent to Ubuntu 20.04 era compilers)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0")
      add_compile_options(-Wno-deprecated-declarations)
      add_compile_options(-Wno-unused-variable)
      message(STATUS "Applied legacy GCC warning suppressions")
    endif()

    # Additional warnings for Clang
    if(UNILINK_COMPILER_CLANG)
      add_compile_options(-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic)
    endif()

    if(UNILINK_ENABLE_WERROR)
      add_compile_options(-Werror)
    endif()
  endif()

  # Visibility settings (disabled for now to avoid linking issues)
  # add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)

  # Optimization flags
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
    if(UNILINK_ENABLE_LTO)
      add_compile_options(-flto)
      add_link_options(-flto)
    endif()
  elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
  endif()

  # Sanitizer support
  if(UNILINK_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(UNILINK_ENABLE_ASAN)
      add_compile_options(-fsanitize=address)
      add_link_options(-fsanitize=address)
    endif()
    if(UNILINK_ENABLE_UBSAN)
      add_compile_options(-fsanitize=undefined)
      add_link_options(-fsanitize=undefined)
    endif()
    if(UNILINK_ENABLE_TSAN)
      add_compile_options(-fsanitize=thread)
      add_link_options(-fsanitize=thread)
    endif()
  endif()

  # Coverage support
  if(UNILINK_ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
  endif()

else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# -----------------------------------------------------------------------------
# Platform-specific settings
# -----------------------------------------------------------------------------

if(WIN32)
  # Unilink-private platform/architecture macros. These are safe to define.
  add_compile_definitions(UNILINK_PLATFORM_WINDOWS=1)

  # Architecture detection:
  # - Prefer CMAKE_SYSTEM_PROCESSOR for distinguishing arm64 vs x64.
  # - Fall back to pointer size when processor hint is not specific.
  set(_UNILINK_IS_ARM64 OFF)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "(ARM64|arm64|aarch64)")
    set(_UNILINK_IS_ARM64 ON)
  endif()

  if(_UNILINK_IS_ARM64)
    add_compile_definitions(UNILINK_ARCH_ARM64=1)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(UNILINK_ARCH_X64=1)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(UNILINK_ARCH_X86=1)
  endif()

  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)

  if(UNILINK_BUILD_SHARED)
    add_compile_definitions(UNILINK_EXPORTS)
  endif()

elseif(UNIX)
  if(APPLE)
    # Enable full BSD/POSIX feature set on macOS (needed for networking macros like NI_MAXHOST)
    add_compile_definitions(_DARWIN_C_SOURCE)
  else()
    add_compile_definitions(_POSIX_C_SOURCE=200809L)
  endif()
endif()
