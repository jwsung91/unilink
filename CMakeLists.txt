cmake_minimum_required(VERSION 3.10)
project(interface_socket)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_EXAMPLES "Build examples" ON)

# Add the src directory, which contains the library's CMakeLists.txt
add_subdirectory(interface_socket)

if(BUILD_EXAMPLES)
  add_executable(echo_tcp_client examples/echo/echo_tcp_client.cc)
  target_link_libraries(echo_tcp_client PRIVATE interface::${PROJECT_NAME})

  add_executable(echo_tcp_server examples/echo/echo_tcp_server.cc)
  target_link_libraries(echo_tcp_server PRIVATE interface::${PROJECT_NAME})

  add_executable(echo_serial examples/echo/echo_serial.cc)
  target_link_libraries(echo_serial PRIVATE interface::${PROJECT_NAME})

  add_executable(chat_tcp_client examples/chat/chat_tcp_client.cc)
  target_link_libraries(chat_tcp_client PRIVATE interface::${PROJECT_NAME})

  add_executable(chat_tcp_server examples/chat/chat_tcp_server.cc)
  target_link_libraries(chat_tcp_server PRIVATE interface::${PROJECT_NAME})

  add_executable(chat_serial examples/chat/chat_serial.cc)
  target_link_libraries(chat_serial PRIVATE interface::${PROJECT_NAME})
endif()
