cmake_minimum_required(VERSION 3.12)
project(conan_test_consumer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the unilink package
find_package(unilink CONFIG REQUIRED)

# Create a simple test executable
add_executable(conan_test_consumer main.cpp)

# Link against unilink
target_link_libraries(conan_test_consumer PRIVATE unilink::unilink)
