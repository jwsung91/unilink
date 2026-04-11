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

  UdpClientBuilder& on_data(std::function<void(const wrapper::MessageContext&)> handler) override;
  UdpClientBuilder& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  UdpClientBuilder& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  UdpClientBuilder& on_error(std::function<void(const wrapper::ErrorContext&)> handler) override;

  UdpClientBuilder& set_local_port(uint16_t port);
  UdpClientBuilder& set_remote(const std::string& address, uint16_t port);
  UdpClientBuilder& use_independent_context(bool use_independent = true);

 private:
  config::UdpConfig cfg_;
  bool auto_manage_;
  bool use_independent_context_;

  std::function<void(const wrapper::MessageContext&)> on_data_;
  std::function<void(const wrapper::ConnectionContext&)> on_connect_;
  std::function<void(const wrapper::ConnectionContext&)> on_disconnect_;
  std::function<void(const wrapper::ErrorContext&)> on_error_;
};

/**
 * @brief Modernized Builder for UdpServer
 */
class UNILINK_API UdpServerBuilder : public BuilderInterface<wrapper::UdpServer, UdpServerBuilder> {
 public:
  UdpServerBuilder();

  std::unique_ptr<wrapper::UdpServer> build() override;

  UdpServerBuilder& auto_manage(bool auto_manage = true) override;

  UdpServerBuilder& on_data(std::function<void(const wrapper::MessageContext&)> handler) override;
  UdpServerBuilder& on_client_connect(std::function<void(const wrapper::ConnectionContext&)> handler);
  UdpServerBuilder& on_client_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler);
  UdpServerBuilder& on_error(std::function<void(const wrapper::ErrorContext&)> handler) override;

  // Implement BuilderInterface requirements by forwarding to client handlers
  UdpServerBuilder& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) override {
    return on_client_connect(std::move(handler));
  }
  UdpServerBuilder& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) override {
    return on_client_disconnect(std::move(handler));
  }

  // Framing support
  using BuilderInterface<wrapper::UdpServer, UdpServerBuilder>::on_message;
  UdpServerBuilder& on_message(std::function<void(const wrapper::MessageContext&)> handler);

  UdpServerBuilder& set_local_port(uint16_t port);
  UdpServerBuilder& use_independent_context(bool use_independent = true);

 private:
  config::UdpConfig cfg_;
  bool auto_manage_;
  bool use_independent_context_;

  std::function<void(const wrapper::MessageContext&)> on_data_;
  std::function<void(const wrapper::ConnectionContext&)> on_connect_;
  std::function<void(const wrapper::ConnectionContext&)> on_disconnect_;
  std::function<void(const wrapper::ErrorContext&)> on_error_;
  std::function<void(const wrapper::MessageContext&)> on_framed_message_;
};

using UdpBuilder = UdpClientBuilder;

}  // namespace builder
}  // namespace unilink
