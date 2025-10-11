# Tutorial Examples CMakeLists.txt
# These examples correspond to the documentation tutorials

cmake_minimum_required(VERSION 3.10)

# Tutorial 1: Getting Started Examples
add_executable(tutorial_simple_client
    getting_started/simple_client.cpp
)
target_link_libraries(tutorial_simple_client
    PRIVATE
        unilink
        Boost::system
        pthread
)

add_executable(tutorial_my_first_client
    getting_started/my_first_client.cpp
)
target_link_libraries(tutorial_my_first_client
    PRIVATE
        unilink
        Boost::system
        pthread
)

# Tutorial 2: TCP Server Examples
add_executable(tutorial_echo_server
    tcp_server/echo_server.cpp
)
target_link_libraries(tutorial_echo_server
    PRIVATE
        unilink
        Boost::system
        pthread
)

add_executable(tutorial_chat_server
    tcp_server/chat_server.cpp
)
target_link_libraries(tutorial_chat_server
    PRIVATE
        unilink
        Boost::system
        pthread
)

# Installation
install(TARGETS
    tutorial_simple_client
    tutorial_my_first_client
    tutorial_echo_server
    tutorial_chat_server
    RUNTIME DESTINATION bin/examples/tutorials
)

