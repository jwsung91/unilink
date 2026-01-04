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

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "unilink/diagnostics/logger.hpp"
#include "unilink/unilink.hpp"

// Example namespace usage - using namespace for simplicity in examples
using namespace unilink;

class EchoServer {
 private:
  std::shared_ptr<wrapper::TcpServer> server_;
  common::Logger& logger_;
  std::atomic<bool> running_;
  unsigned short port_;
  std::atomic<bool> client_connected_;

 public:
  EchoServer(unsigned short port)
      : logger_(unilink::common::Logger::instance()), running_(true), port_(port), client_connected_(false) {
    // Initialize logger
    logger_.set_level(unilink::common::LogLevel::INFO);
    logger_.set_console_output(true);
  }

  ~EchoServer() {
    // Ensure cleanup in destructor
    if (server_) {
      try {
        server_->stop();
        server_.reset();
      } catch (...) {
        // Ignore errors during destruction
      }
    }
  }

  void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      if (running_.load()) {
        logger_.info("server", "signal", "Received shutdown signal");
        running_.store(false);
        // Call shutdown immediately to notify clients
        shutdown();
        // Exit immediately after shutdown to prevent double shutdown
        std::exit(0);
      } else {
        // Force exit if already shutting down
        logger_.warning("server", "signal", "Force exit...");
        std::exit(1);
      }
    }
  }

  void on_multi_connect(size_t client_id, const std::string& client_ip) {
    if (client_connected_.load()) {
      logger_.warning("server", "connect",
                      "Client " + std::to_string(client_id) +
                          " connection rejected - echo server supports only one client at a time");
      // Disconnect the new client immediately
      if (server_) {
        server_->send_to_client(client_id, "[Server] Connection rejected - single client mode");
        // Force disconnect by stopping the server and restarting
        std::thread([this]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          if (server_) {
            server_->stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            server_->start();
          }
        }).detach();
      }
      return;
    }
    client_connected_.store(true);
    logger_.info("server", "connect",
                 "Client " + std::to_string(client_id) + " connected (single client mode): " + client_ip);
  }

  void on_multi_data(size_t client_id, const std::string& data) {
    logger_.info("server", "data", "Client " + std::to_string(client_id) + " message: " + data);

    // Echo back the received data
    if (server_) {
      server_->send_to_client(client_id, data);
      logger_.info("server", "echo", "Echoed to client " + std::to_string(client_id) + ": " + data);
    }
  }

  void on_multi_disconnect(size_t client_id) {
    client_connected_.store(false);
    logger_.info("server", "disconnect", "Client " + std::to_string(client_id) + " disconnected");
  }

  void on_error(const std::string& error) { logger_.error("server", "error", "Error: " + error); }

  bool start() {
    // Create TCP server with new API
    server_ = unilink::tcp_server(port_)
                  .single_client()                   // Use new API
                  .enable_port_retry(true, 3, 1000)  // 3 retries, 1 second interval
                  .on_connect([this](size_t client_id, const std::string& client_ip) {
                    on_multi_connect(client_id, client_ip);
                  })
                  .on_disconnect([this](size_t client_id) { on_multi_disconnect(client_id); })
                  .on_data([this](size_t client_id, const std::string& data) { on_multi_data(client_id, data); })
                  .on_error([this](const std::string& error) { on_error(error); })
                  .build();

    if (!server_) {
      logger_.error("server", "startup", "Failed to create server");
      return false;
    }

    // Start server
    server_->start();

    // Check if server started successfully (considering retry time: 3 retries *
    // 1 second + 0.5 second buffer)
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + (3 * 1000)));

    // Check if server started successfully
    if (!server_->is_listening()) {
      logger_.error("server", "startup", "Failed to start server - port may be in use");
      return false;
    }

    logger_.info("server", "startup", "Server started. Waiting for client connections...");
    return true;
  }

  void process_input() {
    // Non-blocking input processing - more robust method
    if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
      std::string line;
      if (std::getline(std::cin, line)) {
        logger_.info("server", "debug", "Received input: '" + line + "'");
        if (!line.empty()) {
          if (line == "/quit" || line == "/exit") {
            logger_.info("server", "shutdown", "Shutting down server...");
            running_.store(false);
          } else {
            // Send message to client
            logger_.info("server", "debug", "Sending message to client: " + line);
            if (server_) {
              server_->send(line);
            }
            logger_.info("server", "send", "Sent to client: " + line);
          }
        }
      } else {
        // EOF or error on stdin, break the loop
        running_.store(false);
      }
    }
  }

  void run() {
    // Main loop (including input processing)
    while (running_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      process_input();
    }
  }

  void shutdown() {
    // Cleanup
    logger_.info("server", "shutdown", "Shutting down server...");

    if (server_) {
      // Send shutdown notification to clients
      server_->send("[Server] Server is shutting down. Please disconnect.");
      logger_.info("server", "shutdown", "Notified client about shutdown");

      // Give client time to receive message
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Force disconnect all clients first
      try {
        // Send final disconnect message to all clients
        server_->send("[Server] Server is shutting down. Please disconnect.");
        logger_.info("server", "shutdown", "Sent final disconnect message to client");
      } catch (...) {
        logger_.warning("server", "shutdown", "Error sending disconnect message");
      }

      // Stop server
      try {
        server_->stop();
        logger_.info("server", "shutdown", "Server stopped");
      } catch (...) {
        logger_.warning("server", "shutdown", "Error stopping server");
      }

      // Clear server pointer to ensure cleanup
      server_.reset();
      logger_.info("server", "shutdown", "Server pointer cleared");

      // Wait for server cleanup
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    logger_.info("server", "shutdown", "Server shutdown complete");
  }

  void print_info() const {
    std::cout << "=== TCP Echo Server (Single Client) ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;
    std::cout << "Mode: Single client only" << std::endl;
    std::cout << "Exit: Ctrl+C or /quit" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  <message> - Send message to client" << std::endl;
    std::cout << "====================================" << std::endl;
  }

  bool is_running() const { return running_.load(); }
};

// Global pointer for signal handling
EchoServer* g_echo_server = nullptr;

void signal_handler_wrapper(int signal) {
  if (g_echo_server) {
    g_echo_server->signal_handler(signal);
  }
}

int main(int argc, char** argv) {
  unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 8080;

  // Create EchoServer instance on the heap so the signal handler never points to a dangling stack object
  auto echo_server = std::make_unique<EchoServer>(port);
  g_echo_server = echo_server.get();

  // Set up signal handlers
  std::signal(SIGINT, signal_handler_wrapper);
  std::signal(SIGTERM, signal_handler_wrapper);

  // Start the server
  if (!echo_server->start()) {
    return 1;
  }

  echo_server->print_info();

  // Run the main loop
  echo_server->run();

  // Shutdown the server
  echo_server->shutdown();
  g_echo_server = nullptr;

  // Force cleanup and exit
  std::cout << "Server process exiting..." << std::endl;
  return 0;
}
