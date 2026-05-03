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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/**
 * @brief Modernized Builder for TcpClient
 */
template <uint32_t State = BuilderState::None>
class UNILINK_API TcpClientBuilder : public BuilderInterface<wrapper::TcpClient, TcpClientBuilder<State>, State> {
 public:
  template <uint32_t NewState>
  using Rebind = TcpClientBuilder<NewState>;

  TcpClientBuilder(const std::string& host, uint16_t port);

  // Allow conversion between states
  template <uint32_t OtherState>
  TcpClientBuilder(TcpClientBuilder<OtherState>&& other) noexcept
      : host_(std::move(other.host_)),
        port_(other.port_),
        auto_start_(other.auto_start_),
        independent_context_(other.independent_context_),
        retry_interval_(other.retry_interval_),
        max_retries_(other.max_retries_),
        connection_timeout_(other.connection_timeout_) {
    this->on_data_ = std::move(other.on_data_);
    this->on_error_ = std::move(other.on_error_);
    this->on_connect_ = std::move(other.on_connect_);
    this->on_disconnect_ = std::move(other.on_disconnect_);
    this->on_message_ = std::move(other.on_message_);
    this->framer_factory_ = std::move(other.framer_factory_);
    this->bp_strategy_ = other.bp_strategy_;
    this->bp_threshold_ = other.bp_threshold_;
    this->bp_strategy_set_ = other.bp_strategy_set_;
    this->bp_threshold_set_ = other.bp_threshold_set_;
  }

  // Delete copy
  TcpClientBuilder(const TcpClientBuilder&) = delete;
  TcpClientBuilder& operator=(const TcpClientBuilder&) = delete;

  std::unique_ptr<wrapper::TcpClient> build() override;

  TcpClientBuilder<State>& auto_start(bool auto_start = true) override;

  TcpClientBuilder<State>& retry_interval(std::chrono::milliseconds interval);
  TcpClientBuilder<State>& max_retries(int max_retries);
  TcpClientBuilder<State>& connection_timeout(std::chrono::milliseconds timeout);
  TcpClientBuilder<State>& independent_context(bool use_independent = true);

 private:
  template <uint32_t S>
  friend class TcpClientBuilder;

  std::string host_;
  uint16_t port_;
  bool auto_start_;
  bool independent_context_;

  std::chrono::milliseconds retry_interval_;
  int max_retries_;
  std::chrono::milliseconds connection_timeout_;
};

using TcpClientBuilderDefault = TcpClientBuilder<BuilderState::None>;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
