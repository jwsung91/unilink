cmake_minimum_required(VERSION 3.10)
project(interface_socket)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Sources
file(GLOB CONFIGURE_DEPENDS SOURCES "src/*.cc" "src/*.cpp")
file(GLOB CONFIGURE_DEPENDS HEADERS "include/*.h" "include/*.hpp")
add_executable(interface_socket ${SOURCES} ${HEADERS})

target_include_directories(interface_socket PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(interface_socket PRIVATE Boost::system Threads::Threads)
