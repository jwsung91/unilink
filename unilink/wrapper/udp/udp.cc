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

#include "unilink/wrapper/udp/udp.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <iostream>
#include <stdexcept>

#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/udp/udp.hpp"

namespace unilink {
namespace wrapper {

Udp::Udp(const config::UdpConfig& cfg) : cfg_(cfg), channel_(nullptr) {
  // Channel will be created later
}

Udp::Udp(const config::UdpConfig& cfg, std::shared_ptr<boost::asio::io_context> external_ioc)
    : cfg_(cfg),
      channel_(nullptr),
      external_ioc_(std::move(external_ioc)),
      use_external_context_(external_ioc_ != nullptr) {}

Udp::Udp(std::shared_ptr<interface::Channel> channel) : channel_(channel) { setup_internal_handlers(); }

Udp::~Udp() {
  if (started_) {
    stop();
  }
}

void Udp::start() {
  if (started_) return;

  if (use_external_context_) {
    if (!external_ioc_) {
      throw std::runtime_error("External io_context is not set");
    }
    if (manage_external_context_) {
      if (external_ioc_->stopped()) {
        external_ioc_->restart();
      }
    }
  }

  if (!channel_) {
    channel_ = factory::ChannelFactory::create(cfg_, external_ioc_);
    setup_internal_handlers();
  }

  channel_->start();
  if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
    external_thread_ = std::thread([ioc = external_ioc_]() {
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard(ioc->get_executor());
      ioc->run();
    });
  }
  started_ = true;
}

void Udp::stop() {
  if (!started_ || !channel_) return;

  // Clear callbacks to prevent use-after-free
  channel_->on_bytes({});
  channel_->on_state({});
  channel_->on_backpressure({});

  channel_->stop();

  // Manually notify closed state since we detached callbacks
  notify_state_change(base::LinkState::Closed);

  if (use_external_context_ && manage_external_context_) {
    if (external_ioc_) {
      external_ioc_->stop();
    }
    if (external_thread_.joinable()) {
      external_thread_.join();
    }
  }

  started_ = false;
}

void Udp::send(const std::string& data) {
  if (is_connected() && channel_) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    channel_->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}

void Udp::send_line(const std::string& line) { send(line + "\n"); }

bool Udp::is_connected() const { return channel_ && channel_->is_connected(); }

ChannelInterface& Udp::on_data(DataHandler handler) {
  data_handler_ = std::move(handler);
  if (channel_) setup_internal_handlers();
  return *this;
}

ChannelInterface& Udp::on_bytes(BytesHandler handler) {
  bytes_handler_ = std::move(handler);
  if (channel_) setup_internal_handlers();
  return *this;
}

ChannelInterface& Udp::on_connect(ConnectHandler handler) {
  connect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& Udp::on_disconnect(DisconnectHandler handler) {
  disconnect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& Udp::on_error(ErrorHandler handler) {
  error_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& Udp::auto_manage(bool manage) {
  auto_manage_ = manage;
  if (auto_manage_ && !started_) {
    start();
  }
  return *this;
}

void Udp::set_manage_external_context(bool manage) { manage_external_context_ = manage; }

void Udp::setup_internal_handlers() {
  if (!channel_) return;

  channel_->on_bytes([this](memory::ConstByteSpan data) {
    if (bytes_handler_) {
      bytes_handler_(data);
    }
    if (data_handler_) {
      std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
      data_handler_(str_data);
    }
  });

  channel_->on_state([this](base::LinkState state) { notify_state_change(state); });
}

void Udp::notify_state_change(base::LinkState state) {
  switch (state) {
    case base::LinkState::Connected:
      if (connect_handler_) connect_handler_();
      break;
    case base::LinkState::Closed:
      if (disconnect_handler_) disconnect_handler_();
      break;
    case base::LinkState::Error:
      if (error_handler_) error_handler_("Connection error");
      break;
    default:
      break;
  }
}

}  // namespace wrapper
}  // namespace unilink
