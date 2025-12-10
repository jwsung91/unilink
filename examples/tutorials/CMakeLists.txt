# Copyright 2025 Jinwoo Sung
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Tutorial Examples CMakeLists.txt
# These examples correspond to the documentation tutorials

cmake_minimum_required(VERSION 3.10)

# Tutorial 1: Getting Started Examples
add_executable(tutorial_simple_client
    getting_started/simple_client.cpp
)
target_link_libraries(tutorial_simple_client
    PRIVATE
        unilink::unilink
)
unilink_configure_executable(tutorial_simple_client)

add_executable(tutorial_my_first_client
    getting_started/my_first_client.cpp
)
target_link_libraries(tutorial_my_first_client
    PRIVATE
        unilink::unilink
)
unilink_configure_executable(tutorial_my_first_client)

# Tutorial 2: TCP Server Examples
add_executable(tutorial_echo_server
    tcp_server/echo_server.cpp
)
target_link_libraries(tutorial_echo_server
    PRIVATE
        unilink::unilink
)
unilink_configure_executable(tutorial_echo_server)

add_executable(tutorial_chat_server
    tcp_server/chat_server.cpp
)
target_link_libraries(tutorial_chat_server
    PRIVATE
        unilink::unilink
)
unilink_configure_executable(tutorial_chat_server)

# Installation
install(TARGETS
    tutorial_simple_client
    tutorial_my_first_client
    tutorial_echo_server
    tutorial_chat_server
    RUNTIME DESTINATION bin/examples/tutorials
)
