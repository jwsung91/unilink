cmake_minimum_required(VERSION 3.10)
project(interface_socket)
set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

option(ENABLE_YAML_CONFIG "Enable YAML-based runtime config (yaml-cpp)" ON)
if(ENABLE_YAML_CONFIG)
  find_package(yaml-cpp REQUIRED)
  add_definitions(-DINTERFACE_SOCKET_ENABLE_YAML=1)
endif()

file(GLOB_RECURSE SOURCES "src/*.cc")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system Threads::Threads)

if(ENABLE_YAML_CONFIG)
  target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
endif()
