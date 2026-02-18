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

// Example namespace usage
using namespace unilink;

class EchoClient {
 private:
  std::shared_ptr<wrapper::TcpClient> client_;
  diagnostics::Logger& logger_;
  std::atomic<bool> running_;
  std::string host_;
  unsigned short port_;

 public:
  EchoClient(const std::string& host, unsigned short port)
      : logger_(unilink::diagnostics::Logger::instance()), running_(true), host_(host), port_(port) {
    logger_.set_level(unilink::diagnostics::LogLevel::INFO);
    logger_.set_console_output(true);
  }

  ~EchoClient() {
    if (client_) {
      try {
        client_->stop();
      } catch (...) {
      }
    }
  }

  void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      running_.store(false);
    }
  }

  void on_connect(const wrapper::ConnectionContext& ctx) { logger_.info("client", "connect", "Connected to server"); }

  void on_data(const wrapper::MessageContext& ctx) {
    logger_.info("client", "data", "Received: " + std::string(ctx.data()));
  }

  void on_disconnect(const wrapper::ConnectionContext& ctx) {
    logger_.info("client", "disconnect", "Disconnected from server");
    running_.store(false);
  }

  void on_error(const wrapper::ErrorContext& ctx) {
    logger_.error("client", "error", "Error: " + std::string(ctx.message()));
  }

  bool start() {
    client_ = unilink::tcp_client(host_, port_)
                  .retry_interval(1000)
                  .max_retries(5)
                  .on_connect([this](const wrapper::ConnectionContext& ctx) { on_connect(ctx); })
                  .on_disconnect([this](const wrapper::ConnectionContext& ctx) { on_disconnect(ctx); })
                  .on_data([this](const wrapper::MessageContext& ctx) { on_data(ctx); })
                  .on_error([this](const wrapper::ErrorContext& ctx) { on_error(ctx); })
                  .build();

    if (!client_) {
      logger_.error("client", "startup", "Failed to create client");
      return false;
    }

    logger_.info("client", "startup", "Connecting to " + host_ + ":" + std::to_string(port_) + "...");

    // Future API wait
    if (client_->start().get()) {
      logger_.info("client", "startup", "Started and connected.");
      return true;
    }

    logger_.error("client", "startup", "Failed to connect after retries");
    return false;
  }

  void process_input() {
    if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
      std::string line;
      if (std::getline(std::cin, line)) {
        if (line == "/quit" || line == "/exit") {
          running_.store(false);
        } else if (!line.empty()) {
          if (client_ && client_->is_connected()) {
            client_->send(line);
            logger_.info("client", "send", "Sent: " + line);
          } else {
            logger_.warning("client", "send", "Not connected");
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
    if (client_) {
      client_->stop();
      client_.reset();
    }
  }
};

// Global pointer for signal handling
EchoClient* g_echo_client = nullptr;

void signal_handler_wrapper(int signal) {
  if (g_echo_client) {
    g_echo_client->signal_handler(signal);
  }
}

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  unsigned short port = (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 8080;

  auto echo_client = std::make_unique<EchoClient>(host, port);
  g_echo_client = echo_client.get();

  std::signal(SIGINT, signal_handler_wrapper);
  std::signal(SIGTERM, signal_handler_wrapper);

  if (!echo_client->start()) {
    return 1;
  }

  std::cout << "=== TCP Echo Client (Phase 2) ===" << std::endl;
  std::cout << "Commands: <message>, /quit" << std::endl;

  echo_client->run();
  echo_client->shutdown();

  return 0;
}
