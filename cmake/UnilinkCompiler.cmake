# Unilink compiler configuration
# This file handles compiler-specific settings and optimizations

# Set compiler-specific flags
if(MSVC)
  # Microsoft Visual C++
  set(UNILINK_COMPILER_MSVC ON)
  
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
  
  # Common flags for GCC and Clang
  if(UNILINK_ENABLE_WARNINGS)
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    
    # Additional warnings for Clang
    if(UNILINK_COMPILER_CLANG)
      add_compile_options(-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic)
    endif()
    
    if(UNILINK_ENABLE_WERROR)
      add_compile_options(-Werror)
    endif()
  endif()
  
  # Visibility settings
  add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
  
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
  add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
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
