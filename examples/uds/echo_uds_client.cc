/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <chrono>
#include <string>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
  const std::string socket_path = (argc > 1) ? argv[1] : "/tmp/unilink_echo.sock";

  auto client =
      unilink::uds_client(socket_path)
          .on_connect([](const unilink::ConnectionContext&) { std::cout << "Connected to UDS server" << std::endl; })
          .on_disconnect(
              [](const unilink::ConnectionContext&) { std::cout << "Disconnected from UDS server" << std::endl; })
          .on_data([](const unilink::MessageContext& ctx) { std::cout << "[Server] " << ctx.data() << std::endl; })
          .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[Error] " << ctx.message() << std::endl; })
          .build();

  if (!client || !client->start().get()) {
    std::cerr << "Failed to connect to UDS server at " << socket_path << std::endl;
    return 1;
  }

  std::cout << "Connected to " << socket_path << ". Type messages or '/quit'." << std::endl;
  std::string input;
  while (std::getline(std::cin, input)) {
    if (input == "/quit") break;
    client->send(input);
  }

  client->stop();
  return 0;
}
