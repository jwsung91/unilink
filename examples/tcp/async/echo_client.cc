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
#include <memory>
#include <string>

#include "unilink/unilink.hpp"

/**
 * @brief Asynchronous TCP Echo Client Example
 */
int main(int argc, char* argv[]) {
  std::string host = "127.0.0.1";
  uint16_t port = 8080;

  if (argc > 1) host = argv[1];
  if (argc > 2) port = static_cast<uint16_t>(std::stoi(argv[2]));

  std::cout << "--- Async TCP Echo Client ---\n";
  std::cout << "Connecting to " << host << ":" << port << " (non-blocking)\n";

  std::shared_ptr<unilink::wrapper::TcpClient> client;

  auto builder = unilink::tcp_client(host, port);
  builder
      .on_connect([&client](const unilink::ConnectionContext&) {
        std::cout << "\n[Event] Connected to server!\n"
                  << "> " << std::flush;
        if (client) {
          client->send("Auto-greeting from client!");
        }
      })
      .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "\n[Received] " << ctx.data() << "\n"
                  << "> " << std::flush;
      })
      .on_disconnect([](const unilink::ConnectionContext&) {
        std::cout << "\n[Event] Disconnected from server.\n"
                  << "> " << std::flush;
      })
      .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "\n[Error] " << ctx.message() << "\n"
                  << "> " << std::flush;
      });

  client = builder.build();
  client->start();

  std::cout << "Client started in background. Type '/quit' to exit.\n\n";

  std::string line;
  while (true) {
    std::cout << "> " << std::flush;
    if (!std::getline(std::cin, line) || line == "/quit") {
      break;
    }

    if (line.empty()) continue;
    client->send(line);
  }

  client->stop();
  return 0;
}
