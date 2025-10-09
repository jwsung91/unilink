add_executable(echo_tcp_client echo_tcp_client.cc)
target_link_libraries(echo_tcp_client PRIVATE interface::unilink)

add_executable(echo_tcp_server echo_tcp_server.cc)
target_link_libraries(echo_tcp_server PRIVATE interface::unilink)
