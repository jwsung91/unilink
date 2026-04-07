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

#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#include "unilink/config/uds_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/uds/uds_server.hpp"

using namespace unilink;

int main() {
  config::UdsServerConfig cfg;
  cfg.socket_path = "/tmp/unilink_echo.sock";

  auto server = std::static_pointer_cast<transport::UdsServer>(factory::ChannelFactory::create(cfg));

  server->on_multi_connect([](size_t client_id, const std::string& info) {
    std::cout << "Client " << client_id << " connected: " << info << std::endl;
  });

  server->on_multi_data([server](size_t client_id, const std::vector<uint8_t>& data) {
    std::string msg(data.begin(), data.end());
    std::cout << "Received from client " << client_id << ": " << msg << std::endl << std::flush;

    // Echo back
    server->send_to_client(client_id, data);
  });

  server->on_multi_disconnect(
      [](size_t client_id) { std::cout << "Client " << client_id << " disconnected" << std::endl; });

  std::cout << "Starting UDS Echo Server on " << cfg.socket_path << "..." << std::endl;
  server->start();

  std::cout << "Press Enter to stop..." << std::endl;
  std::cin.get();

  server->stop();
  return 0;
}
