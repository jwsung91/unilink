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

#include "unilink/wrapper/serial/serial.hpp"

#include <algorithm>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/serial/serial.hpp"

namespace unilink {
namespace wrapper {

namespace {
std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}
}  // namespace

struct Serial::Impl {
  std::string device;
  uint32_t baud_rate;
  std::shared_ptr<interface::Channel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;

  // Event handlers
  DataHandler data_handler;
  BytesHandler bytes_handler;
  ConnectHandler connect_handler;
  DisconnectHandler disconnect_handler;
  ErrorHandler error_handler;

  // Configuration
  bool auto_manage = false;
  bool started = false;

  // Serial-specific configuration
  int data_bits = 8;
  int stop_bits = 1;
  std::string parity = "none";
  std::string flow_control = "none";
  std::chrono::milliseconds retry_interval{3000};

  Impl(const std::string& dev, uint32_t baud) : device(dev), baud_rate(baud) {}
  Impl(const std::string& dev, uint32_t baud, std::shared_ptr<boost::asio::io_context> ioc)
      : device(dev), baud_rate(baud), external_ioc(std::move(ioc)), use_external_context(external_ioc != nullptr) {}
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

  void setup_internal_handlers(Serial* parent) {
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

  config::SerialConfig build_config() const {
    config::SerialConfig config;
    config.device = device;
    config.baud_rate = baud_rate;
    config.char_size = static_cast<unsigned int>(data_bits);
    config.stop_bits = static_cast<unsigned int>(stop_bits);

    std::string parity_lower = to_lower(parity);
    if (parity_lower == "none")
      config.parity = config::SerialConfig::Parity::None;
    else if (parity_lower == "even")
      config.parity = config::SerialConfig::Parity::Even;
    else if (parity_lower == "odd")
      config.parity = config::SerialConfig::Parity::Odd;

    std::string flow_lower = to_lower(flow_control);
    if (flow_lower == "none")
      config.flow = config::SerialConfig::Flow::None;
    else if (flow_lower == "software")
      config.flow = config::SerialConfig::Flow::Software;
    else if (flow_lower == "hardware")
      config.flow = config::SerialConfig::Flow::Hardware;

    config.retry_interval_ms = static_cast<unsigned int>(retry_interval.count());
    return config;
  }
};

Serial::Serial(const std::string& device, uint32_t baud_rate)
    : pimpl_(std::make_unique<Impl>(device, baud_rate)) {}

Serial::Serial(const std::string& device, uint32_t baud_rate, std::shared_ptr<boost::asio::io_context> external_ioc)
    : pimpl_(std::make_unique<Impl>(device, baud_rate, std::move(external_ioc))) {}

Serial::Serial(std::shared_ptr<interface::Channel> channel) : pimpl_(std::make_unique<Impl>(std::move(channel))) {
  pimpl_->setup_internal_handlers(this);
}

Serial::~Serial() = default;

void Serial::start() {
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
    pimpl_->channel = factory::ChannelFactory::create(pimpl_->build_config(), pimpl_->external_ioc);
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

void Serial::stop() { pimpl_->stop(); }

void Serial::send(std::string_view data) {
  if (is_connected() && pimpl_->channel) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    pimpl_->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}

void Serial::send_line(std::string_view line) { send(std::string(line) + "\n"); }

bool Serial::is_connected() const { return pimpl_->channel && pimpl_->channel->is_connected(); }

ChannelInterface& Serial::on_data(DataHandler handler) {
  pimpl_->data_handler = std::move(handler);
  if (pimpl_->channel) pimpl_->setup_internal_handlers(this);
  return *this;
}

ChannelInterface& Serial::on_bytes(BytesHandler handler) {
  pimpl_->bytes_handler = std::move(handler);
  if (pimpl_->channel) pimpl_->setup_internal_handlers(this);
  return *this;
}

ChannelInterface& Serial::on_connect(ConnectHandler handler) {
  pimpl_->connect_handler = std::move(handler);
  return *this;
}

ChannelInterface& Serial::on_disconnect(DisconnectHandler handler) {
  pimpl_->disconnect_handler = std::move(handler);
  return *this;
}

ChannelInterface& Serial::on_error(ErrorHandler handler) {
  pimpl_->error_handler = std::move(handler);
  return *this;
}

ChannelInterface& Serial::auto_manage(bool manage) {
  pimpl_->auto_manage = manage;
  if (pimpl_->auto_manage && !pimpl_->started) {
    start();
  }
  return *this;
}

void Serial::set_baud_rate(uint32_t baud_rate) { pimpl_->baud_rate = baud_rate; }

void Serial::set_data_bits(int data_bits) { pimpl_->data_bits = data_bits; }

void Serial::set_stop_bits(int stop_bits) { pimpl_->stop_bits = stop_bits; }

void Serial::set_parity(const std::string& parity) { pimpl_->parity = parity; }

void Serial::set_flow_control(const std::string& flow_control) { pimpl_->flow_control = flow_control; }

void Serial::set_retry_interval(std::chrono::milliseconds interval) {
  pimpl_->retry_interval = interval;
  if (pimpl_->channel) {
    auto transport_serial = std::dynamic_pointer_cast<transport::Serial>(pimpl_->channel);
    if (transport_serial) {
      transport_serial->set_retry_interval(static_cast<unsigned int>(interval.count()));
    }
  }
}

config::SerialConfig Serial::build_config() const { return pimpl_->build_config(); }

void Serial::set_manage_external_context(bool manage) { pimpl_->manage_external_context = manage; }

}  // namespace wrapper
}  // namespace unilink
