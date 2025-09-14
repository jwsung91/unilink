cmake_minimum_required(VERSION 3.10)
project(interface_socket)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build examples" ON)

find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cc")
add_library(${PROJECT_NAME} ${SOURCES})
add_library(interface::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::system Threads::Threads)

if(ENABLE_YAML_CONFIG)
  target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
endif()

if(BUILD_EXAMPLES)
  add_executable(echo_client examples/echo_client.cc)
  target_link_libraries(echo_client PRIVATE interface::${PROJECT_NAME})

  add_executable(echo_server examples/echo_server.cc)
  target_link_libraries(echo_server PRIVATE interface::${PROJECT_NAME})

  add_executable(serial examples/serial.cc)
  target_link_libraries(serial PRIVATE interface::${PROJECT_NAME})
  endif()
