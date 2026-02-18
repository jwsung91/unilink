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

#include "unilink/diagnostics/logger.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;

class MultiChatServer {
 public:
  MultiChatServer(uint16_t port)
      : port_(port), logger_(diagnostics::Logger::instance()) {
    logger_.set_console_output(true);
  }

  void run() {
    auto server = unilink::tcp_server(port_)
                      .unlimited_clients()
                      .on_connect([this](const wrapper::ConnectionContext& ctx) {
                        std::string msg = "Client " + std::to_string(ctx.client_id()) + " joined (IP: " + ctx.client_info() + ")";
                        logger_.info("server", "STATE", msg);
                        if (server_) server_->broadcast("*** " + msg + " ***");
                      })
                      .on_disconnect([this](const wrapper::ConnectionContext& ctx) {
                        std::string msg = "Client " + std::to_string(ctx.client_id()) + " left";
                        logger_.info("server", "STATE", msg);
                        if (server_) server_->broadcast("*** " + msg + " ***");
                      })
                      .on_data([this](const wrapper::MessageContext& ctx) {
                        std::string broadcast = "[Client " + std::to_string(ctx.client_id()) + "]: " + std::string(ctx.data());
                        logger_.info("server", "CHAT", broadcast);
                        if (server_) server_->broadcast(broadcast);
                      })
                      .build();

    server_ = std::move(server); // Use std::move to convert unique_ptr to shared_ptr

    if (server->start().get()) {
      logger_.info("server", "main", "Multi-Chat Server started on port " + std::to_string(port_));
    }

    std::cout << "Multi-Chat Server running. Type '/quit' to exit." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      server->broadcast("[Admin]: " + line);
    }

    server->stop();
  }

 private:
  uint16_t port_;
  diagnostics::Logger& logger_;
  std::shared_ptr<wrapper::TcpServer> server_;
};

int main(int argc, char** argv) {
  uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;
  MultiChatServer app(port);
  app.run();
  return 0;
}
