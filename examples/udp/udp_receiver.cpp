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

#include "unilink/unilink.hpp"

using namespace unilink;

int main() {
  // Setup UDP receiver on port 9000
  auto receiver = udp(9000)
                      .on_data([](const wrapper::MessageContext& ctx) {
                        std::cout << "Received UDP: " << ctx.data() << std::endl;
                      })
                      .build();

  if (receiver->start().get()) {
    std::cout << "UDP Receiver listening on port 9000. Press Enter to stop..." << std::endl;
    std::cin.get();
  } else {
    std::cerr << "Failed to start UDP receiver" << std::endl;
  }

  receiver->stop();
  return 0;
}
