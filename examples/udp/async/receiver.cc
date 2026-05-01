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
#include <string>
#include <thread>
#include <chrono>

#include "unilink/unilink.hpp"

/**
 * @brief Asynchronous UDP Receiver Example
 */
int main(int argc, char* argv[]) {
  uint16_t port = 9000;
  if (argc > 1) port = static_cast<uint16_t>(std::stoi(argv[1]));

  std::cout << "--- Async UDP Receiver ---\n";
  std::cout << "Listening on port " << port << " (non-blocking)\n";

  auto receiver = unilink::udp_client(port)
                      .on_data([](const unilink::MessageContext& ctx) {
                        std::cout << "[UDP Recv] from " << ctx.client_info() 
                                  << ": " << ctx.data() << "\n";
                      })
                      .build();

  receiver->start();

  std::cout << "Receiver is active in the background.\n";
  std::cout << "Press Ctrl+C to exit.\n";

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "[Status] Receiver is still listening...\n";
  }

  return 0;
}
