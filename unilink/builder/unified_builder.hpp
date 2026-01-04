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

#pragma once

#include <cstdint>
#include <string>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/serial_builder.hpp"
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/tcp_server_builder.hpp"
#include "unilink/builder/udp_builder.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Unified Builder for creating all types of wrapper instances
 *
 * Provides a single entry point for creating TcpServer, TcpClient, and Serial
 * wrapper instances using a consistent fluent API pattern.
 */
class UNILINK_API UnifiedBuilder {
 public:
  /**
   * @brief Create a TcpServer builder
   * @param port The port number for the server
   * @return TcpServerBuilder A configured builder for TcpServer
   */
  static TcpServerBuilder tcp_server(uint16_t port);

  /**
   * @brief Create a TcpClient builder
   * @param host The host address to connect to
   * @param port The port number to connect to
   * @return TcpClientBuilder A configured builder for TcpClient
   */
  static TcpClientBuilder tcp_client(const std::string& host, uint16_t port);

  /**
   * @brief Create a Serial builder
   * @param device The serial device path (e.g., "/dev/ttyUSB0")
   * @param baud_rate The baud rate for serial communication
   * @return SerialBuilder A configured builder for Serial
   */
  static SerialBuilder serial(const std::string& device, uint32_t baud_rate);

  /**
   * @brief Create a UDP builder
   * @param local_port The local port to bind
   * @return UdpBuilder A configured builder for UDP communication
   */
  static UdpBuilder udp(uint16_t local_port);
};

}  // namespace builder
}  // namespace unilink
