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

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Modernized Builder for TcpServer
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API TcpServerBuilder : public BuilderInterface<wrapper::TcpServer, TcpServerBuilder> {
 public:
  explicit TcpServerBuilder(uint16_t port);

  // Delete copy, allow move
  TcpServerBuilder(const TcpServerBuilder&) = delete;
  TcpServerBuilder& operator=(const TcpServerBuilder&) = delete;
  TcpServerBuilder(TcpServerBuilder&&) = default;
  TcpServerBuilder& operator=(TcpServerBuilder&&) = default;

  /**
   * @brief Build and return the configured TcpServer
   * @return std::unique_ptr<wrapper::TcpServer> The configured server instance
   */
  std::unique_ptr<wrapper::TcpServer> build() override;

  /**
   * @brief Enable auto-manage functionality
   * @param auto_manage Whether to automatically manage the server lifecycle
   * @return TcpServerBuilder& Reference to this builder
   */
  TcpServerBuilder& auto_manage(bool auto_manage = true) override;

  // Modernized event handlers (Override BuilderInterface)
  TcpServerBuilder& on_data(std::function<void(const wrapper::MessageContext&)> handler) override;
  TcpServerBuilder& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  TcpServerBuilder& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  TcpServerBuilder& on_error(std::function<void(const wrapper::ErrorContext&)> handler) override;

  /**
   * @brief Use independent IoContext for this server
   */
  TcpServerBuilder& use_independent_context(bool use_independent = true);

  /**
   * @brief Enable port binding retry on failure
   */
  TcpServerBuilder& enable_port_retry(bool enable = true, int max_retries = 3, int retry_interval_ms = 1000);

  /**
   * @brief Set idle connection timeout
   */
  TcpServerBuilder& idle_timeout(int timeout_ms);

  /**
   * @brief Set maximum number of clients
   */
  TcpServerBuilder& max_clients(size_t max);

  /**
   * @brief Configure server for single client mode
   */
  TcpServerBuilder& single_client();

  /**
   * @brief Configure server for multi-client mode with limit
   */
  TcpServerBuilder& multi_client(size_t max);

  /**
   * @brief Configure server for unlimited multi-client mode
   */
  TcpServerBuilder& unlimited_clients();

 private:
  uint16_t port_;
  bool auto_manage_;
  bool use_independent_context_;

  // Configuration
  bool enable_port_retry_;
  int max_port_retries_;
  int port_retry_interval_ms_;
  int idle_timeout_ms_;
  size_t max_clients_;
  bool client_limit_set_;

  // Modernized callbacks
  std::function<void(const wrapper::MessageContext&)> on_data_;
  std::function<void(const wrapper::ConnectionContext&)> on_connect_;
  std::function<void(const wrapper::ConnectionContext&)> on_disconnect_;
  std::function<void(const wrapper::ErrorContext&)> on_error_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
