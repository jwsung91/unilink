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

#include "unilink/config/serial_config.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/interface/channel.hpp"

namespace unilink {
namespace factory {

/**
 * Channel Factory
 * - 기존 Transport 클래스들 사용
 * - 하위 호환성 유지
 */
class ChannelFactory {
 public:
  using ChannelOptions = std::variant<config::TcpClientConfig, config::TcpServerConfig, config::SerialConfig>;

  // Channel 생성
  static std::shared_ptr<interface::Channel> create(const ChannelOptions& options);

 private:
  // 각 Transport 타입별 생성 함수들
  static std::shared_ptr<interface::Channel> create_tcp_server(const config::TcpServerConfig& cfg);
  static std::shared_ptr<interface::Channel> create_tcp_client(const config::TcpClientConfig& cfg);
  static std::shared_ptr<interface::Channel> create_serial(const config::SerialConfig& cfg);
};

}  // namespace factory
}  // namespace unilink
