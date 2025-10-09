add_executable(chat_tcp_client chat_tcp_client.cc)
target_link_libraries(chat_tcp_client PRIVATE interface::unilink)

add_executable(chat_tcp_server chat_tcp_server.cc)
target_link_libraries(chat_tcp_server PRIVATE interface::unilink)
