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

#include <stdexcept>

#include "unilink/concurrency/io_context_manager.hpp"

namespace unilink {
namespace builder {

TcpServerBuilder::TcpServerBuilder(uint16_t port)
    : port_(port),
      auto_manage_(false),
      use_independent_context_(false),
      enable_port_retry_(false),
      max_port_retries_(3),
      port_retry_interval_ms_(1000),
      idle_timeout_ms_(0),
      max_clients_(0),
      client_limit_set_(false) {}

std::unique_ptr<wrapper::TcpServer> TcpServerBuilder::build() {
  std::unique_ptr<wrapper::TcpServer> server;
  if (use_independent_context_) {
    server = std::make_unique<wrapper::TcpServer>(port_, std::make_shared<boost::asio::io_context>());
    server->set_manage_external_context(true);
  } else {
    server = std::make_unique<wrapper::TcpServer>(port_);
  }

  if (on_data_) server->on_data(on_data_);
  if (on_connect_) server->on_client_connect(on_connect_);
  if (on_disconnect_) server->on_client_disconnect(on_disconnect_);
  if (on_error_) server->on_error(on_error_);

  if (enable_port_retry_) {
    server->enable_port_retry(true, max_port_retries_, port_retry_interval_ms_);
  }

  if (idle_timeout_ms_ > 0) {
    server->idle_timeout(idle_timeout_ms_);
  }

  if (client_limit_set_) {
    if (max_clients_ == 0) server->set_unlimited_clients();
    else server->set_client_limit(max_clients_);
  }

  if (auto_manage_) {
    server->auto_manage(true);
  }

  return server;
}

TcpServerBuilder& TcpServerBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_data(std::function<void(const wrapper::MessageContext&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_error(std::function<void(const wrapper::ErrorContext&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::enable_port_retry(bool enable, int max_retries, int retry_interval_ms) {
  enable_port_retry_ = enable;
  max_port_retries_ = max_retries;
  port_retry_interval_ms_ = retry_interval_ms;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::idle_timeout(int timeout_ms) {
  idle_timeout_ms_ = timeout_ms;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::max_clients(size_t max) {
  if (max == 1) throw std::invalid_argument("Use single_client() for 1 client");
  max_clients_ = max;
  client_limit_set_ = true;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::single_client() {
  max_clients_ = 1; // Simplified: actually handled by TcpServer's set_client_limit(1)
  client_limit_set_ = true;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::multi_client(size_t max) {
  return max_clients(max);
}

TcpServerBuilder& TcpServerBuilder::unlimited_clients() {
  max_clients_ = 0;
  client_limit_set_ = true;
  return *this;
}

}  // namespace builder
}  // namespace unilink
