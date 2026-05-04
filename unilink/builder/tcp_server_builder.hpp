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
#include "unilink/wrapper/tcp_server/tcp_server.hpp"

namespace unilink {
namespace builder {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/**
 * @brief Modernized Builder for TcpServer with C++20 Concepts
 */
template <uint32_t State = BuilderState::None>
class UNILINK_API TcpServerBuilder : public BuilderInterface<wrapper::TcpServer, TcpServerBuilder<State>, State> {
 public:
  template <uint32_t NewState>
  using Rebind = TcpServerBuilder<NewState>;

  explicit TcpServerBuilder(uint16_t port);

  // Allow conversion between states
  template <uint32_t OtherState>
  TcpServerBuilder(TcpServerBuilder<OtherState>&& other) noexcept
      : port_(other.port_),
        bind_address_(std::move(other.bind_address_)),
        auto_start_(other.auto_start_),
        independent_context_(other.independent_context_),
        max_clients_(other.max_clients_),
        client_limit_enabled_(other.client_limit_enabled_),
        port_retry_enabled_(other.port_retry_enabled_),
        max_port_retries_(other.max_port_retries_),
        port_retry_interval_ms_(other.port_retry_interval_ms_),
        idle_timeout_(other.idle_timeout_) {
    this->on_data_ = std::move(other.on_data_);
    this->on_error_ = std::move(other.on_error_);
    this->on_connect_ = std::move(other.on_connect_);
    this->on_disconnect_ = std::move(other.on_disconnect_);
    this->on_message_ = std::move(other.on_message_);
    this->on_data_batch_ = std::move(other.on_data_batch_);
    this->on_message_batch_ = std::move(other.on_message_batch_);
    this->on_backpressure_ = std::move(other.on_backpressure_);
    this->framer_factory_ = std::move(other.framer_factory_);
    this->bp_strategy_ = other.bp_strategy_;
    this->bp_threshold_ = other.bp_threshold_;
    this->bp_strategy_set_ = other.bp_strategy_set_;
    this->bp_threshold_set_ = other.bp_threshold_set_;
  }

  // Delete copy
  TcpServerBuilder(const TcpServerBuilder&) = delete;
  TcpServerBuilder& operator=(const TcpServerBuilder&) = delete;

  std::unique_ptr<wrapper::TcpServer> build() override;

  TcpServerBuilder<State>& auto_start(bool auto_start = true) override;
  TcpServerBuilder<State>& bind_address(const std::string& address);
  TcpServerBuilder<State>& independent_context(bool use_independent = true);
  TcpServerBuilder<State>& max_clients(uint32_t max_clients);
  TcpServerBuilder<State>& enable_port_retry(bool enable = true);
  TcpServerBuilder<State>& max_port_retries(uint32_t max_retries);
  TcpServerBuilder<State>& port_retry_interval(std::chrono::milliseconds interval);

  // Backward compatibility methods
  TcpServerBuilder<State>& port_retry(bool enable = true, int max_retries = 3, int retry_interval_ms = 1000);
  TcpServerBuilder<State>& idle_timeout(std::chrono::milliseconds timeout);
  [[deprecated("Use max_clients(1) instead")]]
  TcpServerBuilder<State>& single_client();
  [[deprecated("Use max_clients(max) instead")]]
  TcpServerBuilder<State>& multi_client(size_t max);

 private:
  template <uint32_t S>
  friend class TcpServerBuilder;

  uint16_t port_;
  std::string bind_address_;
  bool auto_start_;
  bool independent_context_;

  uint32_t max_clients_;
  bool client_limit_enabled_;

  bool port_retry_enabled_;
  uint32_t max_port_retries_;
  uint32_t port_retry_interval_ms_;
  std::chrono::milliseconds idle_timeout_;
};

using TcpServerBuilderDefault = TcpServerBuilder<BuilderState::None>;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
