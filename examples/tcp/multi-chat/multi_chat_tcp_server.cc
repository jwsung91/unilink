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

#include "unilink/common/logger.hpp"
#include "unilink/unilink.hpp"

class ChatServer {
 private:
  std::shared_ptr<unilink::wrapper::TcpServer> server_;
  unilink::common::Logger& logger_;
  std::atomic<bool> running_;
  unsigned short port_;

 public:
  ChatServer(unsigned short port) : logger_(unilink::common::Logger::instance()), running_(true), port_(port) {
    // Initialize logger
    logger_.set_level(unilink::common::LogLevel::INFO);
    logger_.set_console_output(true);
  }

  ~ChatServer() {
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
        // Force immediate shutdown
        shutdown();
        std::exit(0);
      } else {
        // Force exit if already shutting down
        logger_.warning("server", "signal", "Force exit...");
        std::exit(1);
      }
    }
  }

  void on_connect(size_t client_id, const std::string& client_ip) {
    logger_.info("server", "connect", "Client " + std::to_string(client_id) + " connected: " + client_ip);
    logger_.info("server", "debug", "Multi-connect callback triggered for client " + std::to_string(client_id));

    // Broadcast when client connects
    server_->broadcast("[Server] Client " + std::to_string(client_id) + " (" + client_ip + ") joined the chat!");
    logger_.info("server", "broadcast", "Broadcasted client join message for client " + std::to_string(client_id));
  }

  void on_data(size_t client_id, const std::string& data) {
    logger_.info("server", "data", "Client " + std::to_string(client_id) + " message: " + data);
    logger_.info("server", "debug", "Data length: " + std::to_string(data.length()));
    logger_.info("server", "debug", "Data bytes: " + std::to_string(data.size()));

    // Broadcast client message to all clients
    server_->broadcast("[Client " + std::to_string(client_id) + "] " + data);
    logger_.info("server", "broadcast",
                 "Broadcasted message from client " + std::to_string(client_id) + " to all clients");
  }

  void on_disconnect(size_t client_id) {
    logger_.info("server", "disconnect", "Client " + std::to_string(client_id) + " disconnected");

    // Broadcast when client disconnects
    server_->broadcast("[Server] Client " + std::to_string(client_id) + " left the chat!");
    logger_.info("server", "broadcast", "Broadcasted client leave message for client " + std::to_string(client_id));
  }

  bool start() {
    // Create TCP server with 10 client limit
    server_ =
        unilink::tcp_server(port_)
            .multi_client(4)                   // Limit to 4 clients
            .enable_port_retry(true, 3, 1000)  // 3 retries, 1 second interval
            .on_connect([this](size_t client_id, const std::string& client_ip) { on_connect(client_id, client_ip); })
            .on_data([this](size_t client_id, const std::string& data) { on_data(client_id, data); })
            .on_disconnect([this](size_t client_id) { on_disconnect(client_id); })
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

  void setup_broadcast_timer() {
    // Set up timer for broadcast test
    std::thread broadcast_timer([this]() {
      std::this_thread::sleep_for(std::chrono::seconds(5));  // Broadcast after 5 seconds
      logger_.info("server", "debug", "Broadcast timer triggered");
      if (server_) {
        int client_count = server_->get_client_count();
        logger_.info("server", "debug", "Client count: " + std::to_string(client_count));
        if (client_count > 0) {
          server_->broadcast("[Server] Welcome to the chat server!");
          logger_.info("server", "broadcast", "Sent welcome message to all clients");
        } else {
          logger_.info("server", "debug", "No clients connected, skipping broadcast");
        }
      } else {
        logger_.info("server", "debug", "Server is null, skipping broadcast");
      }
    });
    broadcast_timer.detach();
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
          } else if (line == "/clients") {
            int count = server_->get_client_count();
            logger_.info("server", "status", std::to_string(count) + " clients connected");
          } else if (line.substr(0, 5) == "/send") {
            // Process /send <id> <message> format
            size_t first_space = line.find(' ', 6);
            if (first_space != std::string::npos) {
              try {
                size_t client_id = std::stoul(line.substr(6, first_space - 6));
                std::string message = line.substr(first_space + 1);

                // Send to individual client then broadcast
                server_->send_to_client(client_id, message);
                server_->broadcast("[Server] -> Client " + std::to_string(client_id) + ": " + message);
                logger_.info("server", "send", "Sent to client " + std::to_string(client_id) + ": " + message);
                logger_.info("server", "broadcast", "Broadcasted server message to all clients");
              } catch (const std::exception& e) {
                logger_.error("server", "send", "Invalid send command format");
              }
            } else {
              logger_.error("server", "send", "Invalid send command format");
            }
          } else {
            // Broadcast server message
            logger_.info("server", "debug", "Broadcasting server message: " + line);
            server_->broadcast("[Server] " + line);
            logger_.info("server", "broadcast", "Broadcast server message to all clients: " + line);
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
      server_->broadcast("[Server] Server is shutting down. Please disconnect.");
      logger_.info("server", "shutdown", "Notified all clients about shutdown");

      // Give clients time to receive messages
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Force disconnect all clients first
      try {
        // Send final disconnect message to all clients
        server_->broadcast("[Server] Server is shutting down. Please disconnect.");
        logger_.info("server", "shutdown", "Sent final disconnect message to all clients");
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
    std::cout << "=== Multi-Client TCP Chat Server ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;
    std::cout << "Exit: Ctrl+C or /quit" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  /clients - Show connected clients" << std::endl;
    std::cout << "  /send <id> <message> - Send to specific client" << std::endl;
    std::cout << "  <message> - Broadcast to all clients" << std::endl;
    std::cout << "====================================" << std::endl;
  }

  bool is_running() const { return running_.load(); }
};

// Global pointer for signal handling
ChatServer* g_chat_server = nullptr;

void signal_handler_wrapper(int signal) {
  if (g_chat_server) {
    g_chat_server->signal_handler(signal);
  }
}

int main(int argc, char** argv) {
  unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 8080;

  // Create ChatServer instance
  ChatServer chat_server(port);
  g_chat_server = &chat_server;

  // Set up signal handlers
  std::signal(SIGINT, signal_handler_wrapper);
  std::signal(SIGTERM, signal_handler_wrapper);

  // Start the server
  if (!chat_server.start()) {
    return 1;
  }

  chat_server.print_info();

  // Set up broadcast timer
  chat_server.setup_broadcast_timer();

  // Run the main loop
  chat_server.run();

  // Shutdown the server
  chat_server.shutdown();

  // Force cleanup and exit
  std::cout << "Server process exiting..." << std::endl;
  return 0;
}