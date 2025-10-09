project(unilink_examples)

add_executable(echo_tcp_client echo/echo_tcp_client.cc)
target_link_libraries(echo_tcp_client PRIVATE interface::unilink)

add_executable(echo_tcp_server echo/echo_tcp_server.cc)
target_link_libraries(echo_tcp_server PRIVATE interface::unilink)

add_executable(echo_serial echo/echo_serial.cc)
target_link_libraries(echo_serial PRIVATE interface::unilink)

add_executable(chat_tcp_client chat/chat_tcp_client.cc)
target_link_libraries(chat_tcp_client PRIVATE interface::unilink)

add_executable(chat_tcp_server chat/chat_tcp_server.cc)
target_link_libraries(chat_tcp_server PRIVATE interface::unilink)

add_executable(chat_serial chat/chat_serial.cc)
target_link_libraries(chat_serial PRIVATE interface::unilink)

add_executable(logging_example logging_example.cc)
target_link_libraries(logging_example PRIVATE interface::unilink)

add_executable(error_handling_example error_handling_example.cc)
target_link_libraries(error_handling_example PRIVATE interface::unilink)

add_executable(multi_chat_tcp_server multi_chat/multi_chat_tcp_server.cc)
target_link_libraries(multi_chat_tcp_server PRIVATE interface::unilink)

add_executable(multi_chat_tcp_client multi_chat/multi_chat_tcp_client.cc)
target_link_libraries(multi_chat_tcp_client PRIVATE interface::unilink)


