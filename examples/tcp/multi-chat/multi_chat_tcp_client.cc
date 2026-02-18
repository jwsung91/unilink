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

class MultiChatClient {
 public:
  MultiChatClient(const std::string& host, uint16_t port)
      : host_(host), port_(port), logger_(diagnostics::Logger::instance()) {
    logger_.set_console_output(true);
  }

  void run() {
    auto client = unilink::tcp_client(host_, port_)
                      .auto_manage(true)
                      .on_connect([](const wrapper::ConnectionContext& ctx) {
                        std::cout << "\n*** Connected to Multi-Chat Server ***" << std::endl;
                      })
                      .on_data([](const wrapper::MessageContext& ctx) {
                        std::cout << "\n" << ctx.data() << "\n> " << std::flush;
                      })
                      .build();

    std::cout << "Connected. Type messages to send." << std::endl;
    std::cout << "Type '/quit' to exit." << std::endl;

    std::string line;
    std::cout << "> " << std::flush;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      if (client->is_connected()) {
        client->send(line);
        std::cout << "> " << std::flush;
      }
    }

    client->stop();
  }

 private:
  std::string host_;
  uint16_t port_;
  diagnostics::Logger& logger_;
};

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 8080;

  MultiChatClient app(host, port);
  app.run();

  return 0;
}
