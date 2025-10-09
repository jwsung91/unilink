add_executable(chat_serial chat_serial.cc)
target_link_libraries(chat_serial PRIVATE interface::unilink)
