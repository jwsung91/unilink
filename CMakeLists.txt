cmake_minimum_required(VERSION 3.10)
project(interface_socket)
set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cc")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system Threads::Threads)
