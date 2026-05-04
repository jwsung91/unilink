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

#include "unilink/builder/tcp_client_builder.hpp"

#include <boost/asio/io_context.hpp>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/diagnostics/exceptions.hpp"

namespace unilink {
namespace builder {

template <uint32_t State>
TcpClientBuilder<State>::TcpClientBuilder(const std::string& host, uint16_t port)
    : host_(host),
      port_(port),
      auto_start_(false),
      independent_context_(false),
      retry_interval_(3000),
      max_retries_(-1),
      connection_timeout_(5000) {
  if (port == 0) throw diagnostics::BuilderException("Invalid port number: 0");
  if (host.empty()) throw diagnostics::BuilderException("Host cannot be empty");

  // Ensure background IO service is running
  AutoInitializer::ensure_io_context_running();
}

template <uint32_t State>
std::unique_ptr<wrapper::TcpClient> TcpClientBuilder<State>::build() {
#if __cplusplus >= 202002L
  if constexpr (!((State & BuilderState::Ready) == BuilderState::Ready)) {
    throw diagnostics::BuilderException(
        "TcpClientBuilder: Mandatory handlers (on_data/on_message and on_error) must be registered.");
  }
#endif

  std::unique_ptr<wrapper::TcpClient> client;
  if (independent_context_) {
    client = std::make_unique<wrapper::TcpClient>(host_, port_, std::make_shared<boost::asio::io_context>());
    client->manage_external_context(true);
  } else {
    client = std::make_unique<wrapper::TcpClient>(host_, port_);
  }

  if (this->on_data_) client->on_data(this->on_data_);
  if (this->on_data_batch_) client->on_data_batch(this->on_data_batch_);
  if (this->on_connect_) client->on_connect(this->on_connect_);
  if (this->on_disconnect_) client->on_disconnect(this->on_disconnect_);
  if (this->on_error_) client->on_error(this->on_error_);
  if (this->on_backpressure_) client->on_backpressure(this->on_backpressure_);

  client->retry_interval(retry_interval_);
  client->max_retries(max_retries_);
  client->connection_timeout(connection_timeout_);

  if (this->bp_strategy_set_) client->backpressure_strategy(this->bp_strategy_);
  client->backpressure_threshold(this->get_effective_backpressure_threshold());

  if (this->framer_factory_) {
    client->framer(this->framer_factory_());
  }
  if (this->on_message_) {
    client->on_message(std::move(this->on_message_));
  }
  if (this->on_message_batch_) {
    client->on_message_batch(std::move(this->on_message_batch_));
  }

  if (auto_start_) {
    client->auto_start(true);
  }

  return client;
}

template <uint32_t State>
TcpClientBuilder<State>& TcpClientBuilder<State>::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

template <uint32_t State>
TcpClientBuilder<State>& TcpClientBuilder<State>::retry_interval(std::chrono::milliseconds interval) {
  retry_interval_ = interval;
  return *this;
}

template <uint32_t State>
TcpClientBuilder<State>& TcpClientBuilder<State>::max_retries(int max_retries) {
  max_retries_ = max_retries;
  return *this;
}

template <uint32_t State>
TcpClientBuilder<State>& TcpClientBuilder<State>::connection_timeout(std::chrono::milliseconds timeout) {
  connection_timeout_ = timeout;
  return *this;
}

template <uint32_t State>
TcpClientBuilder<State>& TcpClientBuilder<State>::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

// Explicit template instantiations
template class TcpClientBuilder<BuilderState::None>;
template class TcpClientBuilder<BuilderState::HasData>;
template class TcpClientBuilder<BuilderState::HasError>;
template class TcpClientBuilder<BuilderState::Ready>;

}  // namespace builder
}  // namespace unilink
