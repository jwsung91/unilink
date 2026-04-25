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
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 9000;

  auto sender =
      unilink::udp_client(0)
          .remote(host, port)
          .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[error] " << ctx.message() << "\n"; })
          .build();

  if (!sender->start_sync()) {
    std::cerr << "Failed to start UDP sender\n";
    return 1;
  }

  std::cout << "UDP sender ready. Sending to " << host << ":" << port << "\n";
  std::cout << "Type messages or '/quit' to exit.\n";

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "/quit") break;
    sender->send(line);
  }

  sender->stop();
  return 0;
}
