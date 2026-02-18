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
#include <vector>

#include "unilink/unilink.hpp"

/**
 * Echo Server Tutorial
 *
 * Shows how to use ServerInterface, Context objects, and Future-based initialization.
 */

using namespace unilink;

class EchoServer {
 public:
  void start(uint16_t port) {
    server_ = tcp_server(port)
                  .unlimited_clients()
                  .enable_port_retry(true)
                  .on_connect([this](const wrapper::ConnectionContext& ctx) { handle_connect(ctx); })
                  .on_data([this](const wrapper::MessageContext& ctx) { handle_data(ctx); })
                  .build();

    std::cout << "Starting server on port " << port << "..." << std::endl;

    // Future-based result check
    if (server_->start().get()) {
      std::cout << "✓ Server is now listening." << std::endl;
    } else {
      std::cerr << "✗ Server failed to start." << std::endl;
    }
  }

  void handle_connect(const wrapper::ConnectionContext& ctx) {
    std::cout << "[Connect] Client ID: " << ctx.client_id() << " Info: " << ctx.client_info() << std::endl;
    if (server_) {
      server_->send_to(ctx.client_id(), "Welcome to Echo Server!\n");
    }
  }

  void handle_data(const wrapper::MessageContext& ctx) {
    std::cout << "[Data] ID " << ctx.client_id() << ": " << ctx.data() << std::endl;
    if (server_) {
      // Echo back to the specific client
      server_->send_to(ctx.client_id(), "Echo: " + std::string(ctx.data()));
    }
  }

  void stop() {
    if (server_) {
      server_->broadcast("Server shutting down. Goodbye!\n");
      server_->stop();
    }
  }

 private:
  std::shared_ptr<wrapper::TcpServer> server_;
};

int main() {
  EchoServer app;
  app.start(8080);

  std::cout << "Press Enter to stop the server..." << std::endl;
  std::cin.get();

  app.stop();
  return 0;
}
