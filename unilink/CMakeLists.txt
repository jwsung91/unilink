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

file(GLOB_RECURSE LIB_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/interface/*.hpp"
)

add_library(unilink ${LIB_SOURCES})
add_library(interface::unilink ALIAS unilink)

target_include_directories(unilink
  PUBLIC
  # Consumers (e.g., examples) can use #include "unilink/..."
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

# Find and link required packages for the library
find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(unilink PUBLIC Boost::system Threads::Threads)

# Configuration management API (optional)
option(UNILINK_ENABLE_CONFIG "Enable configuration management API" ON)
if(UNILINK_ENABLE_CONFIG)
  target_compile_definitions(unilink PUBLIC UNILINK_ENABLE_CONFIG=1)
endif()

if(ENABLE_YAML_CONFIG)
  target_link_libraries(unilink PRIVATE yaml-cpp)
endif()