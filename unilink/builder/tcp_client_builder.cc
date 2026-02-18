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

namespace unilink {
namespace builder {

TcpClientBuilder::TcpClientBuilder(const std::string& host, uint16_t port)
    : host_(host),
      port_(port),
      auto_manage_(false),
      use_independent_context_(false),
      retry_interval_(3000),
      max_retries_(-1),
      connection_timeout_(5000) {}

std::unique_ptr<wrapper::TcpClient> TcpClientBuilder::build() {
  std::unique_ptr<wrapper::TcpClient> client;
  if (use_independent_context_) {
    client = std::make_unique<wrapper::TcpClient>(host_, port_, std::make_shared<boost::asio::io_context>());
    client->set_manage_external_context(true);
  } else {
    client = std::make_unique<wrapper::TcpClient>(host_, port_);
  }

  if (on_data_) client->on_data(on_data_);
  if (on_connect_) client->on_connect(on_connect_);
  if (on_disconnect_) client->on_disconnect(on_disconnect_);
  if (on_error_) client->on_error(on_error_);

  client->set_retry_interval(retry_interval_);
  client->set_max_retries(max_retries_);
  client->set_connection_timeout(connection_timeout_);

  if (auto_manage_) {
    client->auto_manage(true);
  }

  return client;
}

TcpClientBuilder& TcpClientBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_data(std::function<void(const wrapper::MessageContext&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_error(std::function<void(const wrapper::ErrorContext&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::retry_interval(uint32_t milliseconds) {
  retry_interval_ = std::chrono::milliseconds(milliseconds);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::max_retries(int max_retries) {
  max_retries_ = max_retries;
  return *this;
}

TcpClientBuilder& TcpClientBuilder::connection_timeout(uint32_t milliseconds) {
  connection_timeout_ = std::chrono::milliseconds(milliseconds);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

}  // namespace builder
}  // namespace unilink
