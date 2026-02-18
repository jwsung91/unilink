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

namespace unilink {
namespace builder {

UdpBuilder::UdpBuilder() : auto_manage_(false), use_independent_context_(false) {
  // Ensure background IO service is running
  AutoInitializer::ensure_io_context_running();
}

std::unique_ptr<wrapper::Udp> UdpBuilder::build() {
  std::unique_ptr<wrapper::Udp> udp;
  if (use_independent_context_) {
    udp = std::make_unique<wrapper::Udp>(cfg_, std::make_shared<boost::asio::io_context>());
    udp->set_manage_external_context(true);
  } else {
    udp = std::make_unique<wrapper::Udp>(cfg_);
  }

  if (on_data_) udp->on_data(on_data_);
  if (on_connect_) udp->on_connect(on_connect_);
  if (on_disconnect_) udp->on_disconnect(on_disconnect_);
  if (on_error_) udp->on_error(on_error_);

  if (auto_manage_) {
    udp->auto_manage(true);
  }

  return udp;
}

UdpBuilder& UdpBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

UdpBuilder& UdpBuilder::on_data(std::function<void(const wrapper::MessageContext&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

UdpBuilder& UdpBuilder::on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

UdpBuilder& UdpBuilder::on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

UdpBuilder& UdpBuilder::on_error(std::function<void(const wrapper::ErrorContext&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

UdpBuilder& UdpBuilder::set_local_port(uint16_t port) {
  cfg_.local_port = port;
  return *this;
}

UdpBuilder& UdpBuilder::set_remote(const std::string& address, uint16_t port) {
  cfg_.remote_address = address;
  cfg_.remote_port = port;
  return *this;
}

UdpBuilder& UdpBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

}  // namespace builder
}  // namespace unilink
