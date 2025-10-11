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

/**
 * @file simple_client.cpp
 * @brief Minimal TCP client example (30 seconds to understand)
 *
 * This is the simplest possible unilink client.
 * Perfect for quick testing and learning the basics.
 *
 * Usage:
 *   ./simple_client
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

int main() {
  // Create a TCP client - it's that simple!
  // Note: .build() is now optional - implicit conversion automatically builds
  std::unique_ptr<unilink::wrapper::TcpClient> client =
      unilink::tcp_client("127.0.0.1", 8080)
          .on_connect([]() { std::cout << "Connected!" << std::endl; })
          .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; });

  client->start();

  // Wait for connection
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Send a message
  if (client->is_connected()) {
    client->send("Hello, Server!");
  }

  // Keep running
  std::this_thread::sleep_for(std::chrono::seconds(5));
  return 0;
}
