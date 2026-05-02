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

#include "unilink/builder/udp_builder.hpp"

#include <boost/asio/io_context.hpp>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/diagnostics/exceptions.hpp"

namespace unilink {
namespace builder {

// UdpClientBuilder implementation

template <uint32_t State>
UdpClientBuilder<State>::UdpClientBuilder(uint16_t local_port)
    : local_port_(local_port),
      local_address_("0.0.0.0"),
      remote_host_(""),
      remote_port_(0),
      auto_start_(false),
      independent_context_(false) {
  // Ensure background IO service is running
  AutoInitializer::ensure_io_context_running();
}

template <uint32_t State>
std::unique_ptr<wrapper::UdpClient> UdpClientBuilder<State>::build() {
#if __cplusplus >= 202002L
  if constexpr (!((State & BuilderState::Ready) == BuilderState::Ready)) {
    throw diagnostics::BuilderException("UdpClientBuilder: Mandatory handlers must be set.");
  }
#endif

  std::unique_ptr<wrapper::UdpClient> client;
  config::UdpConfig cfg;
  cfg.local_address = local_address_;
  cfg.local_port = local_port_;
  cfg.remote_address = remote_host_;
  cfg.remote_port = remote_port_;

  if (independent_context_) {
    client = std::make_unique<wrapper::UdpClient>(cfg, std::make_shared<boost::asio::io_context>());
    client->manage_external_context(true);
  } else {
    client = std::make_unique<wrapper::UdpClient>(cfg);
  }

  if (this->on_data_) client->on_data(this->on_data_);
  if (this->on_connect_) client->on_connect(this->on_connect_);
  if (this->on_disconnect_) client->on_disconnect(this->on_disconnect_);
  if (this->on_error_) client->on_error(this->on_error_);

  if (this->bp_strategy_set_) client->backpressure_strategy(this->bp_strategy_);
  client->backpressure_threshold(this->get_effective_backpressure_threshold());

  if (this->framer_factory_) {
    client->framer(this->framer_factory_());
  }
  if (this->on_message_) {
    client->on_message(std::move(this->on_message_));
  }

  if (auto_start_) {
    client->auto_start(true);
  }

  return client;
}

template <uint32_t State>
UdpClientBuilder<State>& UdpClientBuilder<State>::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

template <uint32_t State>
UdpClientBuilder<State>& UdpClientBuilder<State>::local_address(const std::string& address) {
  local_address_ = address;
  return *this;
}

template <uint32_t State>
UdpClientBuilder<State>& UdpClientBuilder<State>::remote_endpoint(const std::string& host, uint16_t port) {
  remote_host_ = host;
  remote_port_ = port;
  return *this;
}

template <uint32_t State>
UdpClientBuilder<State>& UdpClientBuilder<State>::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

// UdpServerBuilder implementation

template <uint32_t State>
UdpServerBuilder<State>::UdpServerBuilder(uint16_t local_port)
    : local_port_(local_port), local_address_("0.0.0.0"), auto_start_(false), independent_context_(false) {
  // Ensure background IO service is running
  AutoInitializer::ensure_io_context_running();
}

template <uint32_t State>
std::unique_ptr<wrapper::UdpServer> UdpServerBuilder<State>::build() {
#if __cplusplus >= 202002L
  if constexpr (!((State & BuilderState::Ready) == BuilderState::Ready)) {
    throw diagnostics::BuilderException("UdpServerBuilder: Mandatory handlers must be set.");
  }
#endif

  std::unique_ptr<wrapper::UdpServer> server;
  config::UdpConfig cfg;
  cfg.local_address = local_address_;
  cfg.local_port = local_port_;

  if (independent_context_) {
    server = std::make_unique<wrapper::UdpServer>(cfg, std::make_shared<boost::asio::io_context>());
    server->manage_external_context(true);
  } else {
    server = std::make_unique<wrapper::UdpServer>(cfg);
  }

  if (this->on_data_) server->on_data(this->on_data_);
  if (this->on_connect_) server->on_connect(this->on_connect_);
  if (this->on_disconnect_) server->on_disconnect(this->on_disconnect_);
  if (this->on_error_) server->on_error(this->on_error_);

  if (this->bp_strategy_set_) server->backpressure_strategy(this->bp_strategy_);
  server->backpressure_threshold(this->get_effective_backpressure_threshold());

  if (this->framer_factory_) {
    // Corrected: ServerInterface::framer expects std::function factory
    server->framer(this->framer_factory_);
  }
  if (this->on_message_) {
    server->on_message(std::move(this->on_message_));
  }

  if (auto_start_) {
    server->auto_start(true);
  }

  return server;
}

template <uint32_t State>
UdpServerBuilder<State>& UdpServerBuilder<State>::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

template <uint32_t State>
UdpServerBuilder<State>& UdpServerBuilder<State>::local_address(const std::string& address) {
  local_address_ = address;
  return *this;
}

template <uint32_t State>
UdpServerBuilder<State>& UdpServerBuilder<State>::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

// Explicit template instantiations
template class UdpClientBuilder<BuilderState::None>;
template class UdpClientBuilder<BuilderState::HasData>;
template class UdpClientBuilder<BuilderState::HasError>;
template class UdpClientBuilder<BuilderState::Ready>;
template class UdpServerBuilder<BuilderState::None>;
template class UdpServerBuilder<BuilderState::HasData>;
template class UdpServerBuilder<BuilderState::HasError>;
template class UdpServerBuilder<BuilderState::Ready>;

}  // namespace builder
}  // namespace unilink
