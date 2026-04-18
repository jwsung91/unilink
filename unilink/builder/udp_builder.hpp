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
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/wrapper/udp/udp.hpp"
#include "unilink/wrapper/udp/udp_server.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Modernized Builder for UdpClient
 */
class UNILINK_API UdpClientBuilder : public BuilderInterface<wrapper::Udp, UdpClientBuilder> {
 public:
  UdpClientBuilder();

  std::unique_ptr<wrapper::Udp> build() override;

  UdpClientBuilder& auto_manage(bool auto_manage = true) override;

  UdpClientBuilder& local_port(uint16_t port);
  UdpClientBuilder& remote(const std::string& address, uint16_t port);
  UdpClientBuilder& independent_context(bool use_independent = true);
  UdpClientBuilder& broadcast(bool enable = true);
  UdpClientBuilder& reuse_address(bool enable = true);

 private:
  config::UdpConfig cfg_;
  bool auto_manage_;
  bool independent_context_;
};

/**
 * @brief Modernized Builder for UdpServer
 */
class UNILINK_API UdpServerBuilder : public BuilderInterface<wrapper::UdpServer, UdpServerBuilder> {
 public:
  UdpServerBuilder();

  std::unique_ptr<wrapper::UdpServer> build() override;

  UdpServerBuilder& auto_manage(bool auto_manage = true) override;

  /**
   * @brief Helper for client connection events
   */
  UdpServerBuilder& on_client_connect(std::function<void(const wrapper::ConnectionContext&)> handler);

  /**
   * @brief Helper for client disconnection events
   */
  UdpServerBuilder& on_client_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler);

  UdpServerBuilder& local_port(uint16_t port);
  UdpServerBuilder& independent_context(bool use_independent = true);
  UdpServerBuilder& broadcast(bool enable = true);
  UdpServerBuilder& reuse_address(bool enable = true);

 private:
  config::UdpConfig cfg_;
  bool auto_manage_;
  bool independent_context_;
};

using UdpBuilder = UdpClientBuilder;

}  // namespace builder
}  // namespace unilink
