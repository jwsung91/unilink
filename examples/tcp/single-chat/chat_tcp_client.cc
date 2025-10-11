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
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

// Example namespace usage - using namespace for simplicity in examples
using namespace unilink;

/**
 * @brief Chat application class for TCP client
 *
 * This class demonstrates member function binding for TCP client chat functionality.
 */
class TcpClientChatApp {
 public:
  TcpClientChatApp(const std::string& host, unsigned short port) : host_(host), port_(port), connected_(false) {}

  void run() {
    // Using convenience function with member function pointers
    auto ul = unilink::tcp_client(host_, port_)
                  .on_connect(this, &TcpClientChatApp::handle_connect)
                  .on_disconnect(this, &TcpClientChatApp::handle_disconnect)
                  .on_data(this, &TcpClientChatApp::handle_data)
                  .build();

    // Start input thread
    std::thread input_thread([this, &ul] { this->input_loop(ul.get()); });

    // Start the client connection
    ul->start();

    // Wait for input thread to finish
    input_thread.join();
    ul->stop();
  }

 private:
  // Member function callbacks - these can be bound directly to the builder
  void handle_connect() {
    common::log_message("client", "STATE", "Connected");
    connected_ = true;
  }

  void handle_disconnect() {
    common::log_message("client", "STATE", "Disconnected");
    connected_ = false;
  }

  void handle_data(const std::string& data) { common::log_message("client", "RX", data); }

  void input_loop(unilink::wrapper::TcpClient* client) {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected_.load()) {
        common::log_message("client", "INFO", "(not connected)");
        continue;
      }
      common::log_message("client", "TX", line);
      client->send_line(line);
    }
  }

  std::string host_;
  unsigned short port_;
  std::atomic<bool> connected_;
};

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  unsigned short port = (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 9000;

  TcpClientChatApp app(host, port);
  app.run();

  return 0;
}
