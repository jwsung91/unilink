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

#include "unilink/diagnostics/logger.hpp"
#include "unilink/unilink.hpp"

/**
 * Logging Example
 *
 * Demonstrates how to use the Unilink logging system with the modern API.
 */

using namespace unilink;

int main() {
  auto& logger = diagnostics::Logger::instance();
  logger.set_level(diagnostics::LogLevel::DEBUG);
  logger.set_console_output(true);

  UNILINK_LOG_INFO("main", "setup", "Starting logging example...");

  // TCP Server with logging
  auto server =
      tcp_server(8080)
          .on_connect([](const unilink::ConnectionContext& ctx) {
            UNILINK_LOG_INFO("tcp_server", "connect", "Client " + std::to_string(ctx.client_id()) + " connected");
          })
          .build();

  // TCP Client with logging
  auto client = tcp_client("127.0.0.1", 8080)
                    .on_connect([](const unilink::ConnectionContext&) {
                      UNILINK_LOG_INFO("tcp_client", "connect", "Connected to server");
                    })
                    .build();

  // Serial with logging
#ifndef _WIN32
  auto serial_dev = unilink::serial("/dev/ttyUSB0", 115200)
                        .on_connect([](const unilink::ConnectionContext&) {
                          UNILINK_LOG_INFO("serial", "connect", "Serial device connected");
                        })
                        .build();
#endif

  UNILINK_LOG_INFO("main", "cleanup", "Example finished.");
  return 0;
}
