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
#include <thread>

#include "unilink/unilink.hpp"

/**
 * My First Unilink Client
 *
 * A comprehensive example showing how to use the modern Phase 2 API
 * with Context objects and Future-based flow control.
 */

using namespace unilink;

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 8080;

  std::cout << "--- Unilink Phase 2 Client Tutorial ---" << std::endl;
  std::cout << "Target: " << host << ":" << port << std::endl;

  // 1. Configure the client using the fluent Builder API
  auto client =
      tcp_client(host, port)
          .retry_interval(2000)
          .max_retries(3)
          .on_connect([](const wrapper::ConnectionContext& ctx) { std::cout << "✓ Connected to server!" << std::endl; })
          .on_disconnect(
              [](const wrapper::ConnectionContext& ctx) { std::cout << "✗ Disconnected from server." << std::endl; })
          .on_data([](const wrapper::MessageContext& ctx) {
            std::cout << "\n[Server] " << ctx.data() << std::endl;
            std::cout << "Enter message: " << std::flush;
          })
          .on_error([](const wrapper::ErrorContext& ctx) {
            std::cerr << "⚠ Error: " << ctx.message() << " (Code: " << static_cast<int>(ctx.code()) << ")" << std::endl;
          })
          .build();

  // 2. Start the client and wait for the result
  std::cout << "Connecting..." << std::endl;
  auto connected = client->start();

  if (connected.get()) {
    std::cout << "Ready! Type your message and press Enter. Type '/quit' to exit." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      client->send(line);
    }
  } else {
    std::cerr << "Failed to connect after retries. Is the server running?" << std::endl;
  }

  // 3. Cleanup happens automatically via RAII or manual stop
  client->stop();
  std::cout << "Goodbye!" << std::endl;

  return 0;
}
