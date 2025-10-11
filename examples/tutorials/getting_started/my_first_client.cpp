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
 * @file my_first_client.cpp
 * @brief Tutorial 1: Getting Started - Your first unilink client
 *
 * This example demonstrates:
 * - Creating a TCP client
 * - Connecting to a server
 * - Sending and receiving data
 * - Handling connection events
 *
 * Usage:
 *   ./my_first_client [server_host] [server_port]
 *
 * Example:
 *   ./my_first_client 127.0.0.1 8080
 *
 * Prerequisites:
 *   A TCP server running on the specified host and port.
 *   You can use netcat: nc -l 8080
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  // Parse command line arguments
  std::string server_host = (argc > 1) ? argv[1] : "127.0.0.1";
  uint16_t server_port = (argc > 2) ? std::stoi(argv[2]) : 8080;

  std::cout << "=== My First Unilink Client ===" << std::endl;
  std::cout << "Connecting to " << server_host << ":" << server_port << std::endl;
  std::cout << std::endl;

  // Step 1: Create a TCP client
  auto client = unilink::tcp_client(server_host, server_port)

                    // Step 2: Handle connection event
                    .on_connect([]() { std::cout << "✓ Connected to server!" << std::endl; })

                    // Step 3: Handle incoming data
                    .on_data([](const std::string& data) { std::cout << "✓ Received: " << data << std::endl; })

                    // Step 4: Handle disconnection
                    .on_disconnect([]() { std::cout << "✗ Disconnected from server" << std::endl; })

                    // Step 5: Handle errors
                    .on_error([](const std::string& error) { std::cerr << "✗ Error: " << error << std::endl; })

                    // Step 6: Configure auto-reconnection
                    .retry_interval(3000)  // Retry every 3 seconds

                    // Build the client
                    .build();

  // Step 7: Start the client
  client->start();

  // Wait for connection (up to 5 seconds)
  std::cout << "Waiting for connection..." << std::endl;
  bool connected = false;
  for (int i = 0; i < 50; ++i) {
    if (client->is_connected()) {
      connected = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  if (!connected) {
    std::cerr << "Failed to connect. Is the server running?" << std::endl;
    std::cerr << "Try: nc -l " << server_port << std::endl;
    return 1;
  }

  // Send a message
  std::cout << "Sending message..." << std::endl;
  client->send("Hello from unilink!");

  // Keep running for 10 seconds
  std::cout << "Running for 10 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(10));

  std::cout << "Shutting down..." << std::endl;
  return 0;
}
