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
  auto client = unilink::tcp_client("127.0.0.1", 8080)
                    .on_connect([]() { std::cout << "Connected!" << std::endl; })
                    .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
                    .auto_start(true)
                    .build();

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
