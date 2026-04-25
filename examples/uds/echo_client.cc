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
#include <string>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  std::string path = (argc > 1) ? argv[1] : "/tmp/unilink_echo.sock";

  auto client =
      unilink::uds_client(path)
          .on_connect(
              [](const unilink::ConnectionContext&) { std::cout << "Connected. Type messages or '/quit' to exit.\n"; })
          .on_data([](const unilink::MessageContext& ctx) { std::cout << "[server] " << ctx.data() << "\n"; })
          .on_disconnect([](const unilink::ConnectionContext&) { std::cout << "Disconnected.\n"; })
          .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[error] " << ctx.message() << "\n"; })
          .build();

  if (!client->start().get()) {
    std::cerr << "Failed to connect to " << path << "\n";
    return 1;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "/quit") break;
    client->send(line);
  }

  client->stop();
  return 0;
}
