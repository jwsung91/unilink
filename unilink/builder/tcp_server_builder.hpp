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
 * @brief Builder for TcpServer wrapper
 *
 * Provides a fluent API for configuring and creating TcpServer instances.
 * Supports method chaining for easy configuration.
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API TcpServerBuilder : public BuilderInterface<wrapper::TcpServer, TcpServerBuilder> {
 public:
  /**
   * @brief Construct a new TcpServerBuilder
   * @param port The port number for the server
   */
  explicit TcpServerBuilder(uint16_t port);

  // Delete copy, allow move
  TcpServerBuilder(const TcpServerBuilder&) = delete;
  TcpServerBuilder& operator=(const TcpServerBuilder&) = delete;
  TcpServerBuilder(TcpServerBuilder&&) = default;
  TcpServerBuilder& operator=(TcpServerBuilder&&) = default;

  using BuilderInterface::on_connect;
  using BuilderInterface::on_data;
  using BuilderInterface::on_disconnect;
  using BuilderInterface::on_error;

  /**
   * @brief Build and return the configured TcpServer
   * @return std::unique_ptr<wrapper::TcpServer> The configured server instance
   */
  std::unique_ptr<wrapper::TcpServer> build() override;

  /**
   * @brief Enable auto-manage functionality
   * @param auto_manage Whether to automatically manage the server lifecycle
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& auto_manage(bool auto_manage = true) override;

  /**
   * @brief Set data handler callback (simple version)
   * @param handler Function to handle incoming data
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_data(std::function<void(const std::string&)> handler) override;

  /**
   * @brief Set data handler callback (with client info)
   * @param handler Function to handle incoming data with client ID
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_data(std::function<void(size_t, const std::string&)> handler);

  /**
   * @brief Set connection handler callback (simple version)
   * @param handler Function to handle connection events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_connect(std::function<void()> handler) override;

  /**
   * @brief Set connection handler callback (with client info)
   * @param handler Function to handle connection events with client ID and IP
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_connect(std::function<void(size_t, const std::string&)> handler);

  /**
   * @brief Set disconnection handler callback (simple version)
   * @param handler Function to handle disconnection events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_disconnect(std::function<void()> handler) override;

  /**
   * @brief Set disconnection handler callback (with client info)
   * @param handler Function to handle disconnection events with client ID
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_disconnect(std::function<void(size_t)> handler);

  /**
   * @brief Set error handler callback
   * @param handler Function to handle error events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_error(std::function<void(const std::string&)> handler) override;

  /**
   * @brief Use independent IoContext for this server (for testing isolation)
   * @param use_independent true to use independent context, false for shared context
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& use_independent_context(bool use_independent = true);

  // Multi-client support methods
  /**
   * @brief Set multi-client connection handler callback
   * @param handler Function to handle multi-client connection events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_multi_connect(std::function<void(size_t, const std::string&)> handler);

  /**
   * @brief Set multi-client data handler callback
   * @param handler Function to handle multi-client data events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_multi_data(std::function<void(size_t, const std::string&)> handler);

  /**
   * @brief Set multi-client disconnection handler callback
   * @param handler Function to handle multi-client disconnection events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_multi_disconnect(std::function<void(size_t)> handler);

  /**
   * @brief Enable port binding retry on failure
   * @param enable Whether to enable retry on port binding failure
   * @param max_retries Maximum number of retry attempts (default: 3)
   * @param retry_interval_ms Retry interval in milliseconds (default: 1000)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& enable_port_retry(bool enable = true, int max_retries = 3, int retry_interval_ms = 1000);

  /**
   * @brief Set idle connection timeout
   * @param timeout_ms Timeout in milliseconds (0 = disabled)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& idle_timeout(int timeout_ms);

  /**
   * @brief Set maximum number of clients (2 or more)
   * @param max Maximum number of clients (must be 2 or more)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   * @throws std::invalid_argument if max is 0 or 1
   */
  TcpServerBuilder& max_clients(size_t max);

  /**
   * @brief Configure server for single client mode
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& single_client();

  /**
   * @brief Configure server for multi-client mode with limit
   * @param max Maximum number of clients (must be 2 or more)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   * @throws std::invalid_argument if max is 0 or 1
   */
  TcpServerBuilder& multi_client(size_t max);

  /**
   * @brief Configure server for unlimited multi-client mode
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& unlimited_clients();

 private:
  uint16_t port_;
  bool auto_manage_;
  bool use_independent_context_;

  // Port retry configuration
  bool enable_port_retry_;
  int max_port_retries_;
  int port_retry_interval_ms_;

  // Idle timeout configuration
  int idle_timeout_ms_;

  // Client limit configuration
  size_t max_clients_;
  bool client_limit_set_;

  std::function<void(const std::string&)> on_data_;
  std::function<void()> on_connect_;
  std::function<void()> on_disconnect_;
  std::function<void(const std::string&)> on_error_;

  // Multi-client callbacks
  std::function<void(size_t, const std::string&)> on_multi_connect_;
  std::function<void(size_t, const std::string&)> on_multi_data_;
  std::function<void(size_t)> on_multi_disconnect_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
