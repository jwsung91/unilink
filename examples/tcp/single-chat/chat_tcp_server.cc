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

#include "unilink/diagnostics/logger.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;

class TcpChatServer {
 public:
  TcpChatServer(uint16_t port)
      : port_(port), logger_(diagnostics::Logger::instance()) {
    logger_.set_console_output(true);
  }

  void run() {
    auto server = unilink::tcp_server(port_)
                      .single_client()
                      .on_connect([this](const wrapper::ConnectionContext& ctx) { handle_connect(ctx); })
                      .on_data([this](const wrapper::MessageContext& ctx) { handle_data(ctx); })
                      .build();

    if (server->start().get()) {
      logger_.info("server", "main", "Server started on port " + std::to_string(port_));
    }

    std::cout << "TCP Single-Client Chat Server started." << std::endl;
    std::cout << "Type messages to broadcast to the connected client." << std::endl;
    std::cout << "Type '/quit' to exit." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      server->broadcast(line);
    }

    server->stop();
  }

 private:
  void handle_connect(const wrapper::ConnectionContext& ctx) {
    logger_.info("server", "STATE", "Client connected: " + ctx.client_info());
  }

  void handle_data(const wrapper::MessageContext& ctx) {
    std::cout << "\n[Client] " << ctx.data() << std::endl;
  }

  uint16_t port_;
  diagnostics::Logger& logger_;
};

int main(int argc, char** argv) {
  uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;
  TcpChatServer app(port);
  app.run();
  return 0;
}
