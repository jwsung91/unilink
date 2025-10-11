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
 * @brief Minimal TCP client example - simplest possible code!
 *
 * This demonstrates the absolute minimum code needed to create
 * a functional TCP client with unilink.
 *
 * Usage:
 *   ./simple_client
 */

#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

int main() {
  // Step 1: Create and configure a TCP client
  auto client = unilink::tcp_client("127.0.0.1", 8080)
                    .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
                    .build();

  // Step 2: Start the connection
  client->start();

  // Step 3: Send data (when connected)
  std::this_thread::sleep_for(std::chrono::seconds(1));
  client->send("Hello, Server!");

  // Step 4: Keep running
  std::this_thread::sleep_for(std::chrono::seconds(5));

  // Step 5: Clean shutdown
  client->stop();
  return 0;
}
