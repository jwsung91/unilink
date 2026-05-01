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

/**
 * @brief Asynchronous UDP Sender Example
 */
int main(int argc, char* argv[]) {
  std::string host = "127.0.0.1";
  uint16_t port = 9000;

  if (argc > 1) host = argv[1];
  if (argc > 2) port = static_cast<uint16_t>(std::stoi(argv[2]));

  std::cout << "--- Async UDP Sender ---\n";
  std::cout << "Target: " << host << ":" << port << "\n";

  auto sender = unilink::udp_client(0) // Bind to any local port
                    .remote(host, port)
                    .build();

  // UDP start is virtually instantaneous
  sender->start();

  std::cout << "Sender ready. Type messages to send via UDP.\n";
  std::cout << "Type '/quit' to exit.\n\n";

  std::string line;
  while (true) {
    std::cout << "> " << std::flush;
    if (!std::getline(std::cin, line) || line == "/quit") break;
    if (line.empty()) continue;

    sender->send(line);
  }

  sender->stop();
  return 0;
}
