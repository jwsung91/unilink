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
#include <thread>

#include "unilink/unilink.hpp"

using namespace unilink;

int main() {
  // Setup UDP sender (point to 127.0.0.1:9000)
  auto sender =
      udp(0)  // Ephemeral local port
          .set_remote("127.0.0.1", 9000)
          .on_connect([](const unilink::ConnectionContext&) { std::cout << "UDP Sender ready" << std::endl; })
          .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "Error: " << ctx.message() << std::endl; })
          .build();

  sender->start();

  std::cout << "Sending messages to 127.0.0.1:9000. Type messages or '/quit'." << std::endl;

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "/quit") break;
    sender->send(line);
  }

  sender->stop();
  return 0;
}
