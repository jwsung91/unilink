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

/**
 * Error Handling Example
 *
 * Demonstrates the modern programmatic error handling using ErrorContext and ErrorCode.
 */

using namespace unilink;

int main() {
  std::cout << "--- Unilink Error Handling Example ---" << std::endl;

  // Attempt to start a server on an invalid port
  auto server = tcp_server(0)
                    .on_error([](const unilink::ErrorContext& ctx) {
                      std::cout << "Server Error Detected!" << std::endl;
                      std::cout << "Code: " << static_cast<int>(ctx.code()) << std::endl;
                      std::cout << "Message: " << ctx.message() << std::endl;

                      if (ctx.code() == ErrorCode::StartFailed) {
                        std::cout << "-> Handling specific start failure..." << std::endl;
                      }
                    })
                    .build();

  // start() returns a future, so we can wait for the error
  auto start_future = server->start();
  if (!start_future.get()) {
    std::cout << "Server start failed as expected." << std::endl;
  }

  // Attempt to connect to a non-existent server
  auto client = tcp_client("127.0.0.1", 1)
                    .on_error([](const unilink::ErrorContext& ctx) {
                      std::cout << "\nClient Error Detected!" << std::endl;
                      std::cout << "Code: " << static_cast<int>(ctx.code()) << std::endl;
                      std::cout << "Message: " << ctx.message() << std::endl;
                    })
                    .build();

  std::cout << "Starting client connection attempt..." << std::endl;
  auto connected = client->start();

  if (!connected.get()) {
    std::cout << "Client connection failed as expected." << std::endl;
  }

  return 0;
}
