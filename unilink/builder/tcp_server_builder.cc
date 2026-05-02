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

#include "unilink/builder/tcp_server_builder.hpp"

#include <boost/asio/io_context.hpp>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/diagnostics/exceptions.hpp"

namespace unilink {
namespace builder {

template <uint32_t State>
TcpServerBuilder<State>::TcpServerBuilder(uint16_t port)
    : port_(port),
      bind_address_("0.0.0.0"),
      auto_start_(false),
      independent_context_(false),
      max_clients_(0),
      client_limit_enabled_(false),
      port_retry_enabled_(false),
      max_port_retries_(10),
      port_retry_interval_ms_(1000),
      idle_timeout_(std::chrono::seconds(30)) {
  if (port == 0) throw diagnostics::BuilderException("Invalid port number: 0");

  // Ensure background IO service is running
  AutoInitializer::ensure_io_context_running();
}

template <uint32_t State>
std::unique_ptr<wrapper::TcpServer> TcpServerBuilder<State>::build() {
#if __cplusplus >= 202002L
  if constexpr (!((State & BuilderState::Ready) == BuilderState::Ready)) {
    static_assert((State & BuilderState::Ready) == BuilderState::Ready,
                  "TcpServerBuilder: Mandatory handlers (on_data and on_error) must be set.");
  }
#endif

  std::unique_ptr<wrapper::TcpServer> server;
  if (independent_context_) {
    server = std::make_unique<wrapper::TcpServer>(port_, std::make_shared<boost::asio::io_context>());
    server->manage_external_context(true);
  } else {
    server = std::make_unique<wrapper::TcpServer>(port_);
  }

  if (this->on_data_) server->on_data(this->on_data_);
  if (this->on_connect_) server->on_connect(this->on_connect_);
  if (this->on_disconnect_) server->on_disconnect(this->on_disconnect_);
  if (this->on_error_) server->on_error(this->on_error_);

  if (client_limit_enabled_) {
    server->max_clients(max_clients_);
  }

  if (this->bp_strategy_set_) server->backpressure_strategy(this->bp_strategy_);
  server->backpressure_threshold(this->get_effective_backpressure_threshold());

  if (this->framer_factory_) {
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
TcpServerBuilder<State>& TcpServerBuilder<State>::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::bind_address(const std::string& address) {
  bind_address_ = address;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::max_clients(uint32_t max_clients) {
  max_clients_ = max_clients;
  client_limit_enabled_ = true;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::enable_port_retry(bool enable) {
  port_retry_enabled_ = enable;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::max_port_retries(uint32_t max_retries) {
  max_port_retries_ = max_retries;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::port_retry_interval(std::chrono::milliseconds interval) {
  port_retry_interval_ms_ = static_cast<uint32_t>(interval.count());
  return *this;
}

// Backward compatibility implementations
template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::port_retry(bool enable, int max_retries, int retry_interval_ms) {
  port_retry_enabled_ = enable;
  max_port_retries_ = static_cast<uint32_t>(max_retries);
  port_retry_interval_ms_ = static_cast<uint32_t>(retry_interval_ms);
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::idle_timeout(std::chrono::milliseconds timeout) {
  idle_timeout_ = timeout;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::single_client() {
  max_clients_ = 1;
  client_limit_enabled_ = true;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::multi_client(size_t max) {
  max_clients_ = static_cast<uint32_t>(max);
  client_limit_enabled_ = true;
  return *this;
}

template <uint32_t State>
TcpServerBuilder<State>& TcpServerBuilder<State>::unlimited_clients() {
  client_limit_enabled_ = false;
  return *this;
}

// Explicit template instantiations
template class TcpServerBuilder<BuilderState::None>;
template class TcpServerBuilder<BuilderState::HasData>;
template class TcpServerBuilder<BuilderState::HasError>;
template class TcpServerBuilder<BuilderState::Ready>;

}  // namespace builder
}  // namespace unilink
