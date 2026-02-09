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
#include <optional>
#include <string>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/wrapper/udp/udp.hpp"

namespace unilink {
namespace builder {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API UdpBuilder : public BuilderInterface<wrapper::Udp, UdpBuilder> {
 public:
  UdpBuilder();

  UdpBuilder(const UdpBuilder&) = delete;
  UdpBuilder& operator=(const UdpBuilder&) = delete;
  UdpBuilder(UdpBuilder&&) = default;
  UdpBuilder& operator=(UdpBuilder&&) = default;

  using BuilderInterface::on_connect;
  using BuilderInterface::on_data;
  using BuilderInterface::on_disconnect;
  using BuilderInterface::on_error;

  std::unique_ptr<wrapper::Udp> build() override;

  UdpBuilder& auto_manage(bool auto_manage = true) override;
  UdpBuilder& on_data(std::function<void(const std::string&)> handler) override;
  UdpBuilder& on_connect(std::function<void()> handler) override;
  UdpBuilder& on_disconnect(std::function<void()> handler) override;
  UdpBuilder& on_error(std::function<void(const std::string&)> handler) override;

  UdpBuilder& set_local_port(uint16_t port);
  UdpBuilder& set_local_address(const std::string& address);
  UdpBuilder& set_remote(const std::string& address, uint16_t port);
  UdpBuilder& use_independent_context(bool use_independent = true);

 private:
  config::UdpConfig cfg_;
  bool auto_manage_{false};
  bool use_independent_context_{false};

  std::function<void(const std::string&)> on_data_;
  std::function<void()> on_connect_;
  std::function<void()> on_disconnect_;
  std::function<void(const std::string&)> on_error_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
