cmake_minimum_required(VERSION 3.10)
project(interface_socket)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build examples" ON)

# Add the src directory, which contains the library's CMakeLists.txt
add_subdirectory(unilink)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
