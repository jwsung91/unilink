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

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::stoi(argv[1])) : 9000;

  auto receiver =
      unilink::udp_client(port)
          .on_data([](const unilink::MessageContext& ctx) { std::cout << "[recv] " << ctx.data() << "\n"; })
          .on_error([](const unilink::ErrorContext& ctx) { std::cerr << "[error] " << ctx.message() << "\n"; })
          .auto_start(true)
          .build();

  std::cout << "UDP receiver listening on port " << port << ". Press Enter to stop.\n";
  std::cin.get();

  receiver->stop();
  return 0;
}
