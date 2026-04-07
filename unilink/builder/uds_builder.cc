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
      auto_manage_(false),
      use_independent_context_(false),
      retry_interval_(3000),
      max_retries_(-1),
      connection_timeout_(5000) {}

std::unique_ptr<wrapper::UdsClient> UdsClientBuilder::build() {
  AutoInitializer::ensure_io_context_running();

  std::unique_ptr<wrapper::UdsClient> client;
  if (use_independent_context_) {
    auto ioc = std::make_shared<boost::asio::io_context>();
    client = std::make_unique<wrapper::UdsClient>(socket_path_, ioc);
    client->set_manage_external_context(true);
  } else {
    client = std::make_unique<wrapper::UdsClient>(socket_path_);
  }

  client->auto_manage(auto_manage_)
      .on_data(on_data_)
      .on_connect(on_connect_)
      .on_disconnect(on_disconnect_)
      .on_error(on_error_);

  client->set_retry_interval(retry_interval_);
  client->set_max_retries(max_retries_);
  client->set_connection_timeout(connection_timeout_);

  return client;
}

UdsClientBuilder& UdsClientBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

UdsClientBuilder& UdsClientBuilder::on_data(std::function<void(const wrapper::MessageContext&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

UdsClientBuilder& UdsClientBuilder::on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

UdsClientBuilder& UdsClientBuilder::on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

UdsClientBuilder& UdsClientBuilder::on_error(std::function<void(const wrapper::ErrorContext&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

UdsClientBuilder& UdsClientBuilder::retry_interval(uint32_t milliseconds) {
  retry_interval_ = std::chrono::milliseconds(milliseconds);
  return *this;
}

UdsClientBuilder& UdsClientBuilder::max_retries(int max_retries) {
  max_retries_ = max_retries;
  return *this;
}

UdsClientBuilder& UdsClientBuilder::connection_timeout(uint32_t milliseconds) {
  connection_timeout_ = std::chrono::milliseconds(milliseconds);
  return *this;
}

UdsClientBuilder& UdsClientBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

// UdsServerBuilder implementation

UdsServerBuilder::UdsServerBuilder(const std::string& socket_path)
    : socket_path_(socket_path), auto_manage_(false), use_independent_context_(false), max_clients_(100) {}

std::unique_ptr<wrapper::UdsServer> UdsServerBuilder::build() {
  AutoInitializer::ensure_io_context_running();

  std::unique_ptr<wrapper::UdsServer> server;
  if (use_independent_context_) {
    auto ioc = std::make_shared<boost::asio::io_context>();
    server = std::make_unique<wrapper::UdsServer>(socket_path_, ioc);
    server->set_manage_external_context(true);
  } else {
    server = std::make_unique<wrapper::UdsServer>(socket_path_);
  }

  server->auto_manage(auto_manage_)
      .on_data(on_data_)
      .on_client_connect(on_connect_)
      .on_client_disconnect(on_disconnect_)
      .on_error(on_error_);

  server->set_client_limit(max_clients_);

  return server;
}

UdsServerBuilder& UdsServerBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

UdsServerBuilder& UdsServerBuilder::on_data(std::function<void(const wrapper::MessageContext&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

UdsServerBuilder& UdsServerBuilder::on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

UdsServerBuilder& UdsServerBuilder::on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

UdsServerBuilder& UdsServerBuilder::on_error(std::function<void(const wrapper::ErrorContext&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

UdsServerBuilder& UdsServerBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
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
