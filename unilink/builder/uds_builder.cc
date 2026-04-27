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

#include "unilink/builder/uds_builder.hpp"

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/concurrency/io_context_manager.hpp"

namespace unilink {
namespace builder {

// UdsClientBuilder implementation

UdsClientBuilder::UdsClientBuilder(const std::string& socket_path)
    : socket_path_(socket_path),
      auto_start_(false),
      independent_context_(false),
      retry_interval_(3000),
      max_retries_(-1),
      connection_timeout_(5000) {}

std::unique_ptr<wrapper::UdsClient> UdsClientBuilder::build() {
  AutoInitializer::ensure_io_context_running();

  std::unique_ptr<wrapper::UdsClient> client;
  if (independent_context_) {
    auto ioc = std::make_shared<boost::asio::io_context>();
    client = std::make_unique<wrapper::UdsClient>(socket_path_, ioc);
    client->manage_external_context(true);
  } else {
    client = std::make_unique<wrapper::UdsClient>(socket_path_);
  }

  if (on_data_) client->on_data(on_data_);
  if (on_connect_) client->on_connect(on_connect_);
  if (on_disconnect_) client->on_disconnect(on_disconnect_);
  if (on_error_) client->on_error(on_error_);

  client->retry_interval(retry_interval_);
  client->max_retries(max_retries_);
  client->connection_timeout(connection_timeout_);

  if (bp_strategy_set_) client->backpressure_strategy(bp_strategy_);
  if (bp_threshold_set_) client->backpressure_threshold(bp_threshold_);

  if (framer_factory_) {
    client->framer(framer_factory_());
  }
  if (on_message_) {
    client->on_message(std::move(on_message_));
  }

  if (auto_start_) {
    client->auto_start(true);
  }

  return client;
}

UdsClientBuilder& UdsClientBuilder::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

UdsClientBuilder& UdsClientBuilder::retry_interval(std::chrono::milliseconds milliseconds) {
  retry_interval_ = milliseconds;
  return *this;
}

UdsClientBuilder& UdsClientBuilder::max_retries(int max_retries) {
  max_retries_ = max_retries;
  return *this;
}

UdsClientBuilder& UdsClientBuilder::connection_timeout(std::chrono::milliseconds milliseconds) {
  connection_timeout_ = milliseconds;
  return *this;
}

UdsClientBuilder& UdsClientBuilder::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

// UdsServerBuilder implementation

UdsServerBuilder::UdsServerBuilder(const std::string& socket_path)
    : socket_path_(socket_path), auto_start_(false), independent_context_(false), max_clients_(100) {}

std::unique_ptr<wrapper::UdsServer> UdsServerBuilder::build() {
  AutoInitializer::ensure_io_context_running();

  std::unique_ptr<wrapper::UdsServer> server;
  if (independent_context_) {
    auto ioc = std::make_shared<boost::asio::io_context>();
    server = std::make_unique<wrapper::UdsServer>(socket_path_, ioc);
    server->manage_external_context(true);
  } else {
    server = std::make_unique<wrapper::UdsServer>(socket_path_);
  }

  if (on_data_) server->on_data(on_data_);
  if (on_connect_) server->on_connect(on_connect_);
  if (on_disconnect_) server->on_disconnect(on_disconnect_);
  if (on_error_) server->on_error(on_error_);

  if (framer_factory_) {
    server->framer(framer_factory_);
  }

  if (on_message_) {
    server->on_message(on_message_);
  }

  server->idle_timeout(idle_timeout_);
  server->max_clients(max_clients_);

  if (bp_strategy_set_) server->backpressure_strategy(bp_strategy_);
  if (bp_threshold_set_) server->backpressure_threshold(bp_threshold_);

  if (auto_start_) {
    server->auto_start(true);
  }

  return server;
}

UdsServerBuilder& UdsServerBuilder::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

UdsServerBuilder& UdsServerBuilder::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

UdsServerBuilder& UdsServerBuilder::idle_timeout(std::chrono::milliseconds timeout) {
  idle_timeout_ = timeout;
  return *this;
}

UdsServerBuilder& UdsServerBuilder::max_clients(size_t max) {
  max_clients_ = max;
  return *this;
}

UdsServerBuilder& UdsServerBuilder::unlimited_clients() {
  max_clients_ = 1000000;
  return *this;
}

}  // namespace builder
}  // namespace unilink
