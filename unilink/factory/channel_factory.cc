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

#include "unilink/factory/channel_factory.hpp"

#include "unilink/transport/serial/serial.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {
namespace factory {

std::shared_ptr<interface::Channel> ChannelFactory::create(const ChannelOptions& options) {
  return std::visit(
      [](const auto& config) -> std::shared_ptr<interface::Channel> {
        using T = std::decay_t<decltype(config)>;

        if constexpr (std::is_same_v<T, config::TcpClientConfig>) {
          return create_tcp_client(config);
        } else if constexpr (std::is_same_v<T, config::TcpServerConfig>) {
          return create_tcp_server(config);
        } else if constexpr (std::is_same_v<T, config::SerialConfig>) {
          return create_serial(config);
        } else {
          static_assert(std::is_same_v<T, void>, "Unsupported config type");
          return nullptr;
        }
      },
      options);
}

std::shared_ptr<interface::Channel> ChannelFactory::create_tcp_server(const config::TcpServerConfig& cfg) {
  return std::make_shared<transport::TcpServer>(cfg);
}

std::shared_ptr<interface::Channel> ChannelFactory::create_tcp_client(const config::TcpClientConfig& cfg) {
  return std::make_shared<transport::TcpClient>(cfg);
}

std::shared_ptr<interface::Channel> ChannelFactory::create_serial(const config::SerialConfig& cfg) {
  return std::make_shared<transport::Serial>(cfg);
}

}  // namespace factory
}  // namespace unilink
