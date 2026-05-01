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

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "unilink/unilink.hpp"

/**
 * @brief Asynchronous Serial Echo Example
 */
int main(int argc, char* argv[]) {
  std::string device = "/dev/ttyUSB0";
  uint32_t baud = 115200;

  if (argc > 1) device = argv[1];
  if (argc > 2) baud = std::stoul(argv[2]);

  std::cout << "--- Async Serial Echo ---\n";
  std::cout << "Opening " << device << " at " << baud << " baud (non-blocking)\n";

  std::shared_ptr<unilink::wrapper::Serial> port;

  auto builder = unilink::serial(device, baud);
  builder
      .on_connect([](const unilink::ConnectionContext&) {
        std::cout << "\n[Event] Serial port opened successfully!\n"
                  << "> " << std::flush;
      })
      .on_data([](const unilink::MessageContext& ctx) {
        std::cout << "\n[Received] " << ctx.data() << "\n"
                  << "> " << std::flush;
      })
      .on_disconnect([](const unilink::ConnectionContext&) {
        std::cout << "\n[Event] Serial port closed/disconnected.\n"
                  << "> " << std::flush;
      })
      .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "\n[Error] " << ctx.message() << "\n"
                  << "> " << std::flush;
      });

  port = builder.build();
  port->start();

  std::cout << "Serial port started in background.\n";
  std::cout << "Main thread is free. Every 5 seconds it will send a heartbeat.\n";
  std::cout << "Type '/quit' to exit.\n\n";

  std::string line;
  // Non-blocking input handling combined with a heartbeat timer
  while (true) {
    std::cout << "> " << std::flush;

    // Simple way to handle input without blocking the whole thread forever,
    // though in a real GUI app this would be even cleaner.
    if (std::cin.peek() != EOF) {
      if (!std::getline(std::cin, line) || line == "/quit") break;
      if (!line.empty()) port->send(line);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  port->stop();
  return 0;
}
