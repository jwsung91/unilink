# Unilink compiler configuration
# This file handles compiler-specific settings and optimizations

# Set compiler-specific flags
if(MSVC)
  # Microsoft Visual C++
  set(UNILINK_COMPILER_MSVC ON)
  add_definitions(-DUNILINK_PLATFORM_WINDOWS=1)
  
  # Remove any existing /W[0-4] flags the generator might have added so we control warning level explicitly.
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
  # Ensure Windows headers detect a target architecture even if the toolchain omits it.
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(_WIN64=1 _AMD64_=1 _M_AMD64=1 _M_X64=1)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(_WIN32=1 _X86_=1 _M_IX86=1)
  endif()

  if(UNILINK_ENABLE_WARNINGS)
    add_compile_options(/W4 /permissive- /utf-8)
    if(UNILINK_ENABLE_WERROR)
      add_compile_options(/WX)
    endif()
  endif()
  
  # MSVC-specific optimizations
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(/O2 /Ob2 /Oi /Ot /Oy /GL)
    add_link_options(/LTCG)
  endif()
  
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

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Ubuntu version detection for compiler-specific settings
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0")
      set(UNILINK_UBUNTU_20_04 ON)
      add_definitions(-DUNILINK_UBUNTU_20_04=1)
      message(STATUS "Detected Ubuntu 20.04 compatibility mode")
    elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
      set(UNILINK_UBUNTU_22_04 ON)
      add_definitions(-DUNILINK_UBUNTU_22_04=1)
      message(STATUS "Detected Ubuntu 22.04 compatibility mode")
    else()
      set(UNILINK_UBUNTU_24_04 ON)
      add_definitions(-DUNILINK_UBUNTU_24_04=1)
      message(STATUS "Detected Ubuntu 24.04+ compatibility mode")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_definitions(-DUNILINK_PLATFORM_MACOS=1)
    message(STATUS "Detected macOS compatibility mode")
  else()
    add_definitions(-DUNILINK_PLATFORM_POSIX=1)
    message(STATUS "Detected generic POSIX compatibility mode")
  endif()
  
  # Common flags for GCC and Clang
  if(UNILINK_ENABLE_WARNINGS)
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    
    # Ubuntu 20.04 specific warning suppressions
    if(UNILINK_UBUNTU_20_04)
      add_compile_options(-Wno-deprecated-declarations)
      add_compile_options(-Wno-unused-variable)
      message(STATUS "Applied Ubuntu 20.04 warning suppressions")
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

# Set C++ standard (will be set later when target is created)

# Platform-specific settings
if(WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0A00)
  if(UNILINK_BUILD_SHARED)
    add_definitions(-DUNILINK_EXPORTS)
  endif()
elseif(UNIX)
  add_definitions(-D_POSIX_C_SOURCE=200809L)
endif()

# Debug information
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(UNILINK_COMPILER_MSVC)
    add_compile_options(/Zi)
  else()
    add_compile_options(-g3)
  endif()
endif()
