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

#include <chrono>
#include <cstdint>
#include <string>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Modernized Builder for TcpClient
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API TcpClientBuilder : public BuilderInterface<wrapper::TcpClient, TcpClientBuilder> {
 public:
  TcpClientBuilder(const std::string& host, uint16_t port);

  // Delete copy, allow move
  TcpClientBuilder(const TcpClientBuilder&) = delete;
  TcpClientBuilder& operator=(const TcpClientBuilder&) = delete;
  TcpClientBuilder(TcpClientBuilder&&) = default;
  TcpClientBuilder& operator=(TcpClientBuilder&&) = default;

  std::unique_ptr<wrapper::TcpClient> build() override;

  TcpClientBuilder& auto_manage(bool auto_manage = true) override;

  // Modernized event handlers
  TcpClientBuilder& on_data(std::function<void(const wrapper::MessageContext&)> handler) override;
  TcpClientBuilder& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  TcpClientBuilder& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  TcpClientBuilder& on_error(std::function<void(const wrapper::ErrorContext&)> handler) override;

  /**
   * @brief Set connection retry interval
   */
  TcpClientBuilder& retry_interval(uint32_t milliseconds);

  /**
   * @brief Set maximum connection retries (-1 for unlimited)
   */
  TcpClientBuilder& max_retries(int max_retries);

  /**
   * @brief Set connection timeout
   */
  TcpClientBuilder& connection_timeout(uint32_t milliseconds);

  /**
   * @brief Use independent IoContext for this client
   */
  TcpClientBuilder& use_independent_context(bool use_independent = true);

 private:
  std::string host_;
  uint16_t port_;
  bool auto_manage_;
  bool use_independent_context_;

  // Configuration
  std::chrono::milliseconds retry_interval_;
  int max_retries_;
  std::chrono::milliseconds connection_timeout_;

  // Callbacks
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
