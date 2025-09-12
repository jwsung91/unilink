cmake_minimum_required(VERSION 3.10)
project(class_example)

file(GLOB SOURCES "src/*.cc" "include/*.h")
add_executable(class_example ${SOURCES})
target_include_directories(class_example PUBLIC ${CMAKE_SOURCE_DIR}/include)
