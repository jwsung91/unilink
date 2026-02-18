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
  diagnostics::Logger& logger_;
  std::atomic<bool> running_;
  unsigned short port_;
  std::atomic<bool> client_connected_;

 public:
  EchoServer(unsigned short port)
      : logger_(unilink::diagnostics::Logger::instance()), running_(true), port_(port), client_connected_(false) {
    // Initialize logger
    logger_.set_level(unilink::diagnostics::LogLevel::INFO);
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
        shutdown();
        std::exit(0);
      } else {
        logger_.warning("server", "signal", "Force exit...");
        std::exit(1);
      }
    }
  }

  void on_client_connect(const wrapper::ConnectionContext& ctx) {
    if (client_connected_.load()) {
      logger_.warning("server", "connect",
                      "Client " + std::to_string(ctx.client_id()) +
                          " connection rejected - echo server supports only one client at a time");
      if (server_) {
        server_->send_to(ctx.client_id(), "[Server] Connection rejected - single client mode");
        // Disconnect logic could be more robust in production
      }
      return;
    }
    client_connected_.store(true);
    logger_.info("server", "connect",
                 "Client " + std::to_string(ctx.client_id()) + " connected: " + ctx.client_info());
  }

  void on_data(const wrapper::MessageContext& ctx) {
    logger_.info("server", "data", "Client " + std::to_string(ctx.client_id()) + " message: " + std::string(ctx.data()));

    // Echo back the received data
    if (server_) {
      server_->send_to(ctx.client_id(), ctx.data());
      logger_.info("server", "echo", "Echoed to client " + std::to_string(ctx.client_id()));
    }
  }

  void on_client_disconnect(const wrapper::ConnectionContext& ctx) {
    client_connected_.store(false);
    logger_.info("server", "disconnect", "Client " + std::to_string(ctx.client_id()) + " disconnected");
  }

  void on_error(const wrapper::ErrorContext& ctx) { 
    logger_.error("server", "error", "Error [" + std::to_string(static_cast<int>(ctx.code())) + "]: " + std::string(ctx.message())); 
  }

  bool start() {
    // Create TCP server with Phase 2 Modern API
    server_ = unilink::tcp_server(port_)
                  .single_client()
                  .enable_port_retry(true, 3, 1000)
                  .on_connect([this](const wrapper::ConnectionContext& ctx) { on_client_connect(ctx); })
                  .on_disconnect([this](const wrapper::ConnectionContext& ctx) { on_client_disconnect(ctx); })
                  .on_data([this](const wrapper::MessageContext& ctx) { on_data(ctx); })
                  .on_error([this](const wrapper::ErrorContext& ctx) { on_error(ctx); })
                  .build();

    if (!server_) {
      logger_.error("server", "startup", "Failed to create server");
      return false;
    }

    // Start server and WAIT for result using Future API
    logger_.info("server", "startup", "Starting server on port " + std::to_string(port_) + "...");
    auto start_future = server_->start();
    
    if (!start_future.get()) { // Blocking wait for start result
      logger_.error("server", "startup", "Failed to start server - port may be in use or other IO error");
      return false;
    }

    logger_.info("server", "startup", "Server started successfully. Waiting for client connections...");
    return true;
  }

  void process_input() {
    if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
      std::string line;
      if (std::getline(std::cin, line)) {
        if (!line.empty()) {
          if (line == "/quit" || line == "/exit") {
            running_.store(false);
          } else {
            if (server_) {
              server_->broadcast(line);
              logger_.info("server", "broadcast", "Broadcasted to all clients: " + line);
            }
          }
        }
      } else {
        running_.store(false);
      }
    }
  }

  void run() {
    while (running_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      process_input();
    }
  }

  void shutdown() {
    logger_.info("server", "shutdown", "Shutting down server...");
    if (server_) {
      server_->broadcast("[Server] Server is shutting down.");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      server_->stop();
      server_.reset();
    }
    logger_.info("server", "shutdown", "Server shutdown complete");
  }

  void print_info() const {
    std::cout << "=== TCP Echo Server (Phase 2 Modern API) ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;
    std::cout << "Exit: Ctrl+C or /quit" << std::endl;
    std::cout << "============================================" << std::endl;
  }
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

  auto echo_server = std::make_unique<EchoServer>(port);
  g_echo_server = echo_server.get();

  std::signal(SIGINT, signal_handler_wrapper);
  std::signal(SIGTERM, signal_handler_wrapper);

  if (!echo_server->start()) {
    return 1;
  }

  echo_server->print_info();
  echo_server->run();
  echo_server->shutdown();
  
  return 0;
}
