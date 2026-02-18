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

class SerialChatApp {
 public:
  SerialChatApp(const std::string& device, uint32_t baud_rate)
      : device_(device), baud_rate_(baud_rate), logger_(diagnostics::Logger::instance()) {
    logger_.set_console_output(true);
  }

  void run() {
    auto ul = unilink::serial(device_, baud_rate_)
                  .on_connect([this](const wrapper::ConnectionContext& ctx) { handle_connect(); })
                  .on_disconnect([this](const wrapper::ConnectionContext& ctx) { handle_disconnect(); })
                  .on_data([this](const wrapper::MessageContext& ctx) { handle_data(std::string(ctx.data())); })
                  .on_error([this](const wrapper::ErrorContext& ctx) { handle_error(std::string(ctx.message())); })
                  .build();

    ul->start();

    // Use a shared pointer to ensure the 'ul' object stays alive for the thread
    auto shared_ul = std::shared_ptr<wrapper::Serial>(std::move(ul));
    std::thread input_thread([this, shared_ul] { this->input_loop(shared_ul.get()); });

    std::cout << "Serial Chat started. Type messages to send." << std::endl;
    std::cout << "Type '/quit' to exit." << std::endl;

    input_thread.join();
    shared_ul->stop();
  }

 private:
  void handle_connect() {
    logger_.info("serial", "STATE", "Serial device connected");
  }

  void handle_disconnect() {
    logger_.info("serial", "STATE", "Serial device disconnected");
  }

  void handle_data(const std::string& data) {
    std::cout << "\n[RX] " << data << "\n> " << std::flush;
  }

  void handle_error(const std::string& error) {
    logger_.error("serial", "ERROR", error);
  }

  void input_loop(unilink::wrapper::Serial* ul) {
    std::string line;
    std::cout << "> " << std::flush;
    while (std::getline(std::cin, line)) {
      if (line == "/quit") break;
      if (ul && ul->is_connected()) {
        ul->send(line);
        std::cout << "> " << std::flush;
      } else {
        logger_.warning("serial", "INFO", "(not connected)");
        std::cout << "> " << std::flush;
      }
    }
  }

  std::string device_;
  uint32_t baud_rate_;
  diagnostics::Logger& logger_;
};

int main(int argc, char** argv) {
  std::string device = (argc > 1) ? argv[1] : "/dev/ttyUSB0";
  uint32_t baud = (argc > 2) ? static_cast<uint32_t>(std::stoul(argv[2])) : 115200;

  SerialChatApp app(device, baud);
  app.run();

  return 0;
}
