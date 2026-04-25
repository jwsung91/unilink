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

// Uses a class so that server_ is a stable member that callbacks can safely
// capture via `this` without any dangling-pointer risk.
class EchoServer {
 public:
  explicit EchoServer(uint16_t port) : port_(port) {}

  bool start() {
    server_ =
        unilink::tcp_server(port_)
            .unlimited_clients()
            .on_connect([](const unilink::ConnectionContext& ctx) {
              std::cout << "[connect] client " << ctx.client_id() << " from " << ctx.client_info() << "\n";
            })
            .on_data([this](const unilink::MessageContext& ctx) { server_->send_to(ctx.client_id(), ctx.data()); })
            .on_disconnect([](const unilink::ConnectionContext& ctx) {
              std::cout << "[disconnect] client " << ctx.client_id() << "\n";
            })
            .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[error] " << ctx.message() << "\n"; })
            .build();

    if (!server_->start().get()) {
      std::cerr << "Failed to start server on port " << port_ << "\n";
      return false;
    }

    std::cout << "Echo server listening on port " << port_ << "\n";
    return true;
  }

  void run() {
    std::cout << "Press Enter to stop.\n";
    std::cin.get();
  }

  void stop() {
    if (server_) server_->stop();
  }

 private:
  uint16_t port_;
  std::unique_ptr<unilink::TcpServer> server_;
};

int main(int argc, char** argv) {
  uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;

  EchoServer server(port);
  if (!server.start()) return 1;

  server.run();
  server.stop();
  return 0;
}
