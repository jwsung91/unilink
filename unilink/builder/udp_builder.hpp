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
#include "unilink/builder/ibuilder.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/wrapper/udp/udp.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Modernized Builder for Udp
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API UdpBuilder : public BuilderInterface<wrapper::Udp, UdpBuilder> {
 public:
  UdpBuilder();

  // Delete copy, allow move
  UdpBuilder(const UdpBuilder&) = delete;
  UdpBuilder& operator=(const UdpBuilder&) = delete;
  UdpBuilder(UdpBuilder&&) = default;
  UdpBuilder& operator=(UdpBuilder&&) = default;

  std::unique_ptr<wrapper::Udp> build() override;

  UdpBuilder& auto_manage(bool auto_manage = true) override;

  // Modernized event handlers
  UdpBuilder& on_data(std::function<void(const wrapper::MessageContext&)> handler) override;
  UdpBuilder& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  UdpBuilder& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  UdpBuilder& on_error(std::function<void(const wrapper::ErrorContext&)> handler) override;

  /**
   * @brief Set local port to bind
   */
  UdpBuilder& set_local_port(uint16_t port);

  /**
   * @brief Set remote address and port
   */
  UdpBuilder& set_remote(const std::string& address, uint16_t port);

  /**
   * @brief Use independent IoContext
   */
  UdpBuilder& use_independent_context(bool use_independent = true);

 private:
  config::UdpConfig cfg_;
  bool auto_manage_;
  bool use_independent_context_;

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
