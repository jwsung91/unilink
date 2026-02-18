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
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/udp/udp.hpp"

namespace unilink {
namespace wrapper {

struct Udp::Impl {
  config::UdpConfig cfg;
  std::shared_ptr<interface::Channel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;

  DataHandler data_handler;
  BytesHandler bytes_handler;
  ConnectHandler connect_handler;
  DisconnectHandler disconnect_handler;
  ErrorHandler error_handler;

  bool auto_manage{false};
  bool started{false};

  explicit Impl(const config::UdpConfig& config) : cfg(config) {}
  Impl(const config::UdpConfig& config, std::shared_ptr<boost::asio::io_context> ioc)
      : cfg(config), external_ioc(std::move(ioc)), use_external_context(external_ioc != nullptr) {}
  explicit Impl(std::shared_ptr<interface::Channel> ch) : channel(std::move(ch)) {}

  ~Impl() {
    if (started) {
      stop();
    }
  }

  void stop() {
    if (!started || !channel) return;

    channel->stop();

    if (use_external_context && manage_external_context) {
      if (external_ioc) {
        external_ioc->stop();
      }
      if (external_thread.joinable()) {
        external_thread.join();
      }
    }

    started = false;
  }

  void setup_internal_handlers(Udp* parent) {
    if (!channel) return;

    channel->on_bytes([this](memory::ConstByteSpan data) {
      if (bytes_handler) {
        bytes_handler(data);
      }
      if (data_handler) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler(str_data);
      }
    });

    channel->on_state([this](base::LinkState state) { notify_state_change(state); });
  }

  void notify_state_change(base::LinkState state) {
    switch (state) {
      case base::LinkState::Connected:
        if (connect_handler) connect_handler();
        break;
      case base::LinkState::Closed:
        if (disconnect_handler) disconnect_handler();
        break;
      case base::LinkState::Error:
        if (error_handler) error_handler("Connection error");
        break;
      default:
        break;
    }
  }
};

Udp::Udp(const config::UdpConfig& cfg) : pimpl_(std::make_unique<Impl>(cfg)) {}

Udp::Udp(const config::UdpConfig& cfg, std::shared_ptr<boost::asio::io_context> external_ioc)
    : pimpl_(std::make_unique<Impl>(cfg, std::move(external_ioc))) {}

Udp::Udp(std::shared_ptr<interface::Channel> channel) : pimpl_(std::make_unique<Impl>(std::move(channel))) {
  pimpl_->setup_internal_handlers(this);
}

Udp::~Udp() = default;

void Udp::start() {
  if (pimpl_->started) return;

  if (pimpl_->use_external_context) {
    if (!pimpl_->external_ioc) {
      throw std::runtime_error("External io_context is not set");
    }
    if (pimpl_->manage_external_context) {
      if (pimpl_->external_ioc->stopped()) {
        pimpl_->external_ioc->restart();
      }
    }
  }

  if (!pimpl_->channel) {
    pimpl_->channel = factory::ChannelFactory::create(pimpl_->cfg, pimpl_->external_ioc);
    pimpl_->setup_internal_handlers(this);
  }

  pimpl_->channel->start();
  if (pimpl_->use_external_context && pimpl_->manage_external_context && !pimpl_->external_thread.joinable()) {
    pimpl_->external_thread = std::thread([ioc = pimpl_->external_ioc]() {
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard(ioc->get_executor());
      ioc->run();
    });
  }
  pimpl_->started = true;
}

void Udp::stop() { pimpl_->stop(); }

void Udp::send(std::string_view data) {
  if (is_connected() && pimpl_->channel) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    pimpl_->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}

void Udp::send_line(std::string_view line) { send(std::string(line) + "\n"); }

bool Udp::is_connected() const { return pimpl_->channel && pimpl_->channel->is_connected(); }

ChannelInterface& Udp::on_data(DataHandler handler) {
  pimpl_->data_handler = std::move(handler);
  if (pimpl_->channel) pimpl_->setup_internal_handlers(this);
  return *this;
}

ChannelInterface& Udp::on_bytes(BytesHandler handler) {
  pimpl_->bytes_handler = std::move(handler);
  if (pimpl_->channel) pimpl_->setup_internal_handlers(this);
  return *this;
}

ChannelInterface& Udp::on_connect(ConnectHandler handler) {
  pimpl_->connect_handler = std::move(handler);
  return *this;
}

ChannelInterface& Udp::on_disconnect(DisconnectHandler handler) {
  pimpl_->disconnect_handler = std::move(handler);
  return *this;
}

ChannelInterface& Udp::on_error(ErrorHandler handler) {
  pimpl_->error_handler = std::move(handler);
  return *this;
}

ChannelInterface& Udp::auto_manage(bool manage) {
  pimpl_->auto_manage = manage;
  if (pimpl_->auto_manage && !pimpl_->started) {
    start();
  }
  return *this;
}

void Udp::set_manage_external_context(bool manage) { pimpl_->manage_external_context = manage; }

}  // namespace wrapper
}  // namespace unilink
