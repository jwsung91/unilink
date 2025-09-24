cmake_minimum_required(VERSION 3.10)
project(unilink)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(unilink)

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    GTest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip # release-1.10.0
  )
  FetchContent_MakeAvailable(GTest)
  add_subdirectory(test)
endif()
