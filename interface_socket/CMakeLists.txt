file(GLOB_RECURSE LIB_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
)

add_library(${PROJECT_NAME} ${LIB_SOURCES})
add_library(interface::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Find and link required packages for the library
find_package(Boost 1.70 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC Boost::system Threads::Threads)

if(ENABLE_YAML_CONFIG)
  target_link_libraries(${PROJECT_NAME} PRIVATE yaml-cpp)
endif()