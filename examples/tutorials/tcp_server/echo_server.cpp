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
 * @file echo_server.cpp
 * @brief Tutorial 2: Basic echo server
 *
 * This example demonstrates:
 * - Creating a TCP server
 * - Accepting client connections
 * - Echoing received data back to clients
 * - Tracking connected clients
 *
 * Usage:
 *   ./echo_server [port]
 *
 * Example:
 *   ./echo_server 8080
 *
 * Test with:
 *   telnet localhost 8080
 *   or
 *   nc localhost 8080
 */

#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "\nReceived shutdown signal..." << std::endl;
    g_running.store(false);
  }
}

class EchoServer {
 private:
  std::shared_ptr<unilink::wrapper::TcpServer> server_;
  std::atomic<int> client_count_{0};
  uint16_t port_;

 public:
  EchoServer(uint16_t port) : port_(port) {}

  void start() {
    std::cout << "Starting echo server on port " << port_ << std::endl;

    server_ = unilink::tcp_server(port_)
                  .on_connect([this](size_t client_id, const std::string& ip) { handle_connect(client_id, ip); })
                  .on_data([this](size_t client_id, const std::string& data) { handle_data(client_id, data); })
                  .on_disconnect([this](size_t client_id) { handle_disconnect(client_id); })
                  .on_error([this](const std::string& error) { handle_error(error); })
                  .auto_start(true)
                  .build();

    std::cout << "Server started! Waiting for connections..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
  }

  void handle_connect(size_t client_id, const std::string& ip) {
    client_count_++;
    std::cout << "[Client " << client_id << "] Connected from " << ip << " (Total clients: " << client_count_ << ")"
              << std::endl;

    // Send welcome message
    server_->send_to_client(client_id, "Welcome to Echo Server!\n");
  }

  void handle_data(size_t client_id, const std::string& data) {
    std::cout << "[Client " << client_id << "] Received: " << data;

    // Echo back the data
    server_->send_to_client(client_id, "Echo: " + data);
  }

  void handle_disconnect(size_t client_id) {
    client_count_--;
    std::cout << "[Client " << client_id << "] Disconnected " << "(Remaining clients: " << client_count_ << ")"
              << std::endl;
  }

  void handle_error(const std::string& error) { std::cerr << "[Error] " << error << std::endl; }

  void stop() {
    if (server_) {
      std::cout << "Stopping server..." << std::endl;
      server_->send("Server shutting down. Goodbye!\n");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      server_->stop();
    }
  }

  bool is_running() const { return server_ && server_->is_listening(); }
};

int main(int argc, char** argv) {
  // Set up signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // Parse port
  uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8080;

  // Create and start server
  EchoServer server(port);
  server.start();

  // Wait for shutdown signal
  while (g_running.load() && server.is_running()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  // Cleanup
  server.stop();
  std::cout << "Server stopped." << std::endl;

  return 0;
}
