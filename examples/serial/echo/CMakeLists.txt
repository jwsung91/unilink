add_executable(echo_serial echo_serial.cc)
target_link_libraries(echo_serial PRIVATE interface::unilink)
