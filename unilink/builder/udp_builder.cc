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
#include <memory>

#include "unilink/concurrency/io_context_manager.hpp"

namespace unilink {
namespace builder {

// --- UdpClientBuilder Implementation ---

UdpClientBuilder::UdpClientBuilder() : auto_manage_(false), independent_context_(false) {}

std::unique_ptr<wrapper::Udp> UdpClientBuilder::build() {
  std::shared_ptr<boost::asio::io_context> ioc = nullptr;
  if (independent_context_) {
    ioc = std::make_shared<boost::asio::io_context>();
  }

  auto udp = std::make_unique<wrapper::Udp>(cfg_, ioc);
  if (independent_context_) {
    udp->manage_external_context(true);
  }

  if (on_data_) udp->on_data(on_data_);
  if (on_connect_) udp->on_connect(on_connect_);
  if (on_disconnect_) udp->on_disconnect(on_disconnect_);
  if (on_error_) udp->on_error(on_error_);

  if (framer_factory_) {
    udp->framer(framer_factory_());
  }
  if (on_message_) {
    udp->on_message(std::move(on_message_));
  }

  if (auto_manage_) {
    udp->auto_manage(true);
  }

  return udp;
}

UdpClientBuilder& UdpClientBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

UdpClientBuilder& UdpClientBuilder::local_port(uint16_t port) {
  cfg_.local_port = port;
  return *this;
}

UdpClientBuilder& UdpClientBuilder::remote(const std::string& address, uint16_t port) {
  cfg_.remote_address = address;
  cfg_.remote_port = port;
  return *this;
}

UdpClientBuilder& UdpClientBuilder::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

UdpClientBuilder& UdpClientBuilder::broadcast(bool enable) {
  cfg_.enable_broadcast = enable;
  return *this;
}

UdpClientBuilder& UdpClientBuilder::reuse_address(bool enable) {
  cfg_.reuse_address = enable;
  return *this;
}

// --- UdpServerBuilder Implementation ---

UdpServerBuilder::UdpServerBuilder() : auto_manage_(false), independent_context_(false) {}

std::unique_ptr<wrapper::UdpServer> UdpServerBuilder::build() {
  std::shared_ptr<boost::asio::io_context> ioc = nullptr;
  if (independent_context_) {
    ioc = std::make_shared<boost::asio::io_context>();
  }

  auto server = std::make_unique<wrapper::UdpServer>(cfg_, ioc);
  if (independent_context_) {
    server->manage_external_context(true);
  }

  if (on_data_) server->on_data(on_data_);
  if (on_connect_) server->on_client_connect(on_connect_);
  if (on_disconnect_) server->on_client_disconnect(on_disconnect_);
  if (on_error_) server->on_error(on_error_);

  if (framer_factory_) {
    server->framer_factory(framer_factory_);
  }

  if (on_message_) {
    server->on_message(on_message_);
  }

  if (auto_manage_) {
    server->start();
  }

  return server;
}

UdpServerBuilder& UdpServerBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

UdpServerBuilder& UdpServerBuilder::local_port(uint16_t port) {
  cfg_.local_port = port;
  return *this;
}

UdpServerBuilder& UdpServerBuilder::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

UdpServerBuilder& UdpServerBuilder::broadcast(bool enable) {
  cfg_.enable_broadcast = enable;
  return *this;
}

UdpServerBuilder& UdpServerBuilder::reuse_address(bool enable) {
  cfg_.reuse_address = enable;
  return *this;
}

}  // namespace builder
}  // namespace unilink
