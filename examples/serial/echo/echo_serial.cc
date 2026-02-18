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

class SerialEchoApp {
 public:
  SerialEchoApp(const std::string& device, uint32_t baud_rate)
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

    if (ul->start().get()) {
      logger_.info("serial", "main", "Serial started successfully");
    }

    std::thread sender_thread([this, &ul] { this->sender_loop(ul.get()); });

    std::cout << "Serial Echo started. Type something..." << std::endl;
    std::cout << "Press Enter with empty message to exit." << std::endl;

    sender_thread.join();
    ul->stop();
  }

 private:
  void handle_connect() {
    logger_.info("serial", "STATE", "Serial device connected");
  }

  void handle_disconnect() {
    logger_.info("serial", "STATE", "Serial device disconnected");
  }

  void handle_data(const std::string& data) {
    logger_.info("serial", "RX", data);
  }

  void handle_error(const std::string& error) {
    logger_.error("serial", "ERROR", error);
  }

  void sender_loop(unilink::wrapper::Serial* ul) {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty()) break;
      if (ul && ul->is_connected()) {
        ul->send(line);
        logger_.info("serial", "TX", line);
      } else {
        std::cout << "(Not connected)" << std::endl;
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

  SerialEchoApp app(device, baud);
  app.run();

  return 0;
}
