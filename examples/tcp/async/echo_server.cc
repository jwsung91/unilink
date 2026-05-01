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

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

/**
 * @brief Asynchronous TCP Echo Server Example
 */
int main(int argc, char* argv[]) {
  uint16_t port = 8080;
  if (argc > 1) port = static_cast<uint16_t>(std::stoi(argv[1]));

  std::cout << "--- Async TCP Echo Server ---\n";

  // Use a shared_ptr to allow capture in callbacks
  // Note: build() returns unique_ptr, so we move it to shared_ptr
  std::shared_ptr<unilink::wrapper::TcpServer> server;

  auto builder = unilink::tcp_server(port);
  builder.unlimited_clients()
      .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "[Server] Client connected: ID=" << ctx.client_id() << "\n";
      })
      .on_data([&server](const unilink::MessageContext& ctx) {
        std::cout << "[Server] Data from " << ctx.client_id() << ": " << ctx.data() << "\n";
        if (server) {
          server->send_to(ctx.client_id(), ctx.data());
        }
      })
      .on_disconnect([](const unilink::ConnectionContext& ctx) {
        std::cout << "[Server] Client disconnected: ID=" << ctx.client_id() << "\n";
      })
      .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[Server Error] " << ctx.message() << "\n"; });

  server = builder.build();

  server->start();

  std::cout << "Server started on port " << port << " (non-blocking)\n";
  std::cout << "The main thread will now display status updates every 5 seconds.\n";
  std::cout << "Press Ctrl+C to stop.\n\n";

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (server) {
      auto clients = server->connected_clients();
      std::cout << "[Status] Heartbeat: " << clients.size() << " client(s) connected.\n";
    }
  }

  return 0;
}
