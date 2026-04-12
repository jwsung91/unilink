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
#include <string>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
  const std::string socket_path = (argc > 1) ? argv[1] : "/tmp/unilink_echo.sock";

  std::unique_ptr<unilink::UdsServer> server;
  server =
      unilink::uds_server(socket_path)
          .unlimited_clients()
          .on_connect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client connected: " << ctx.client_id() << " info=" << ctx.client_info() << std::endl;
          })
          .on_data([&server](const unilink::MessageContext& ctx) {
            std::cout << "Received: " << ctx.data() << std::endl;
            if (server) {
              server->send_to(ctx.client_id(), ctx.data());
            }
          })
          .on_disconnect([](const unilink::ConnectionContext& ctx) {
            std::cout << "Client disconnected: " << ctx.client_id() << std::endl;
          })
          .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[Error] " << ctx.message() << std::endl; })
          .build();

  if (!server || !server->start().get()) {
    std::cerr << "Failed to start UDS server on " << socket_path << std::endl;
    return 1;
  }

  std::cout << "UDS echo server listening on " << socket_path << ". Press Enter to stop..." << std::endl;
  std::cin.get();

  server->stop();
  return 0;
}
