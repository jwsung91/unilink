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

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "unilink/diagnostics/logger.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;

class TcpClientChatApp {
 public:
  TcpClientChatApp(const std::string& host, uint16_t port)
      : host_(host), port_(port), logger_(diagnostics::Logger::instance()) {
    logger_.set_console_output(true);
  }

  void run() {
    auto ul = unilink::tcp_client(host_, port_)
                  .on_connect([this](const unilink::ConnectionContext&) { handle_connect(); })
                  .on_disconnect([this](const unilink::ConnectionContext&) { handle_disconnect(); })
                  .on_data([this](const unilink::MessageContext& ctx) { handle_data(std::string(ctx.data())); })
                  .on_error([this](const unilink::ErrorContext& ctx) { handle_error(std::string(ctx.message())); })
                  .build();

    ul->start();

    auto shared_ul = std::shared_ptr<unilink::TcpClient>(std::move(ul));
    std::thread input_thread([this, shared_ul] { this->input_loop(shared_ul.get()); });

    std::cout << "TCP Chat Client started. Type messages to send." << std::endl;
    std::cout << "Type '/quit' to exit." << std::endl;

    input_thread.join();
    shared_ul->stop();
  }

 private:
  void handle_connect() { logger_.info("client", "STATE", "Connected"); }

  void handle_disconnect() { logger_.info("client", "STATE", "Disconnected"); }

  void handle_data(const std::string& data) { std::cout << "\n[Server] " << data << "\n> " << std::flush; }

  void handle_error(const std::string& error) { logger_.error("client", "ERROR", error); }

  void input_loop(unilink::TcpClient* ul) {
    std::string line;
    std::cout << "> " << std::flush;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      if (ul && ul->is_connected()) {
        ul->send(line);
        std::cout << "> " << std::flush;
      } else {
        logger_.warning("client", "INFO", "(not connected)");
        std::cout << "> " << std::flush;
      }
    }
  }

  std::string host_;
  uint16_t port_;
  diagnostics::Logger& logger_;
};

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 8080;

  TcpClientChatApp app(host, port);
  app.run();

  return 0;
}
