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

// Multi-client server that broadcasts every message to all connected clients.
// Connect multiple echo_client instances to see the broadcast in action.
class BroadcastServer {
 public:
  explicit BroadcastServer(uint16_t port) : port_(port) {}

  bool start() {
    server_ = unilink::tcp_server(port_)
                  .unlimited_clients()
                  .on_connect([this](const unilink::ConnectionContext& ctx) {
                    std::string msg = "client " + std::to_string(ctx.client_id()) + " joined";
                    std::cout << "[connect] " << msg << " from " << ctx.client_info() << "\n";
                    server_->broadcast("*** " + msg + " ***");
                  })
                  .on_data([this](const unilink::MessageContext& ctx) {
                    std::string msg = "[" + std::to_string(ctx.client_id()) + "] " + std::string(ctx.data());
                    std::cout << msg << "\n";
                    server_->broadcast(msg);
                  })
                  .on_disconnect([this](const unilink::ConnectionContext& ctx) {
                    std::string msg = "client " + std::to_string(ctx.client_id()) + " left";
                    std::cout << "[disconnect] " << msg << "\n";
                    server_->broadcast("*** " + msg + " ***");
                  })
                  .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[error] " << ctx.message() << "\n"; })
                  .build();

    if (!server_->start().get()) {
      std::cerr << "Failed to start server on port " << port_ << "\n";
      return false;
    }

    std::cout << "Broadcast server listening on port " << port_ << "\n";
    return true;
  }

  void run() {
    std::cout << "Type messages to broadcast to all clients, or '/quit' to stop.\n";
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      server_->broadcast("[server] " + line);
    }
  }

  void stop() {
    if (server_) {
      server_->broadcast("Server shutting down.");
      server_->stop();
    }
  }

 private:
  uint16_t port_;
  std::unique_ptr<unilink::TcpServer> server_;
};

int main(int argc, char** argv) {
  uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;

  BroadcastServer server(port);
  if (!server.start()) return 1;

  server.run();
  server.stop();
  return 0;
}
