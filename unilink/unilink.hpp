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

#include <memory>
#include <variant>

#include "unilink/base/deprecated.hpp"
#include "unilink/base/error_codes.hpp"
#include "unilink/base/platform.hpp"
#include "unilink/base/visibility.hpp"

// Public API Context and Interface headers
#include "unilink/wrapper/context.hpp"
#include "unilink/wrapper/ichannel.hpp"
#include "unilink/wrapper/iserver.hpp"

// Wrapper implementations
#include "unilink/wrapper/serial/serial.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include "unilink/wrapper/udp/udp.hpp"

// High-level Builder API includes
#include "unilink/builder/ibuilder.hpp"
#include "unilink/builder/serial_builder.hpp"
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/tcp_server_builder.hpp"
#include "unilink/builder/udp_builder.hpp"
#include "unilink/builder/unified_builder.hpp"

// Configuration Management API includes (optional)
#ifdef UNILINK_ENABLE_CONFIG
#include "unilink/config/config_factory.hpp"
#include "unilink/config/config_manager.hpp"
#include "unilink/config/iconfig_manager.hpp"
#endif

// Error handling and logging system includes
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"

namespace unilink {

// === Public API Namespaces ===

// Builder API namespace
namespace builder {
class TcpServerBuilder;
class TcpClientBuilder;
class SerialBuilder;
class UdpBuilder;
class UnifiedBuilder;
}  // namespace builder

// Wrapper API namespace
namespace wrapper {
class TcpServer;
class TcpClient;
class Serial;
class Udp;
class ChannelInterface;
class ServerInterface;
class MessageContext;
class ConnectionContext;
class ErrorContext;
}  // namespace wrapper

// === Public Facade Aliases ===

// Core communication classes
using TcpClient = wrapper::TcpClient;
using TcpServer = wrapper::TcpServer;
using Serial = wrapper::Serial;
using Udp = wrapper::Udp;

// Context classes for callbacks
using MessageContext = wrapper::MessageContext;
using ConnectionContext = wrapper::ConnectionContext;
using ErrorContext = wrapper::ErrorContext;

// === Public Builder API Convenience Functions ===

/**
 * @brief Create a TCP server builder
 * @param port The port number for the server
 * @return TcpServerBuilder A configured builder for TcpServer
 */
inline builder::TcpServerBuilder tcp_server(uint16_t port) { return builder::TcpServerBuilder(port); }

/**
 * @brief Create a TCP client builder
 * @param host The host address to connect to
 * @param port The port number to connect to
 * @return TcpClientBuilder A configured builder for TcpClient
 */
inline builder::TcpClientBuilder tcp_client(const std::string& host, uint16_t port) {
  return builder::TcpClientBuilder(host, port);
}

/**
 * @brief Create a Serial port builder
 * @param device The serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate The baud rate for serial communication
 * @return SerialBuilder A configured builder for Serial
 */
inline builder::SerialBuilder serial(const std::string& device, uint32_t baud_rate) {
  return builder::SerialBuilder(device, baud_rate);
}

/**
 * @brief Create a UDP builder
 * @param local_port The local port to bind
 * @return UdpBuilder A configured builder for UDP
 */
inline builder::UdpBuilder udp(uint16_t local_port) {
  builder::UdpBuilder b;
  b.set_local_port(local_port);
  return b;
}

}  // namespace unilink
