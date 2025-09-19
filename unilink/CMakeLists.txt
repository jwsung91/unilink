file(GLOB_RECURSE LIB_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
)

add_library(unilink ${LIB_SOURCES})
add_library(interface::unilink ALIAS unilink)

target_include_directories(
  unilink PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Find and link required packages for the library
find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(unilink PUBLIC Boost::system Threads::Threads)

if(ENABLE_YAML_CONFIG)
  target_link_libraries(unilink PRIVATE yaml-cpp)
endif()