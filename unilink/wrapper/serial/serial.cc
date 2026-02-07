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
#include <cctype>
#include <iostream>
#include <stdexcept>

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

Serial::Serial(const std::string& device, uint32_t baud_rate)
    : device_(device), baud_rate_(baud_rate), channel_(nullptr) {
  // Channel will be created later
}

Serial::Serial(const std::string& device, uint32_t baud_rate, std::shared_ptr<boost::asio::io_context> external_ioc)
    : device_(device),
      baud_rate_(baud_rate),
      channel_(nullptr),
      external_ioc_(std::move(external_ioc)),
      use_external_context_(external_ioc_ != nullptr) {}

Serial::Serial(std::shared_ptr<interface::Channel> channel) : device_(""), baud_rate_(0), channel_(channel) {
  setup_internal_handlers();
}

Serial::~Serial() {
  if (started_) {
    stop();
  }
}

void Serial::start() {
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
    channel_ = factory::ChannelFactory::create(build_config(), external_ioc_);
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

void Serial::stop() {
  if (!started_ || !channel_) return;

  channel_->stop();

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

void Serial::send(const std::string& data) {
  if (is_connected() && channel_) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    channel_->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}

void Serial::send_line(const std::string& line) { send(line + "\n"); }

bool Serial::is_connected() const { return channel_ && channel_->is_connected(); }

ChannelInterface& Serial::on_data(DataHandler handler) {
  data_handler_ = std::move(handler);
  if (channel_) setup_internal_handlers();
  return *this;
}

ChannelInterface& Serial::on_bytes(BytesHandler handler) {
  bytes_handler_ = std::move(handler);
  if (channel_) setup_internal_handlers();
  return *this;
}

ChannelInterface& Serial::on_connect(ConnectHandler handler) {
  connect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& Serial::on_disconnect(DisconnectHandler handler) {
  disconnect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& Serial::on_error(ErrorHandler handler) {
  error_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& Serial::auto_manage(bool manage) {
  auto_manage_ = manage;
  if (auto_manage_ && !started_) {
    start();
  }
  return *this;
}

void Serial::set_baud_rate(uint32_t baud_rate) { baud_rate_ = baud_rate; }

void Serial::set_data_bits(int data_bits) { data_bits_ = data_bits; }

void Serial::set_stop_bits(int stop_bits) { stop_bits_ = stop_bits; }

void Serial::set_parity(const std::string& parity) { parity_ = parity; }

void Serial::set_flow_control(const std::string& flow_control) { flow_control_ = flow_control; }

void Serial::set_retry_interval(std::chrono::milliseconds interval) {
  retry_interval_ = interval;
  if (channel_) {
    auto transport_serial = std::dynamic_pointer_cast<transport::Serial>(channel_);
    if (transport_serial) {
      transport_serial->set_retry_interval(static_cast<unsigned int>(interval.count()));
    }
  }
}

config::SerialConfig Serial::build_config() const {
  config::SerialConfig config;
  config.device = device_;
  config.baud_rate = baud_rate_;
  config.char_size = data_bits_;
  config.stop_bits = stop_bits_;

  std::string parity_lower = to_lower(parity_);
  if (parity_lower == "none")
    config.parity = config::SerialConfig::Parity::None;
  else if (parity_lower == "even")
    config.parity = config::SerialConfig::Parity::Even;
  else if (parity_lower == "odd")
    config.parity = config::SerialConfig::Parity::Odd;

  std::string flow_lower = to_lower(flow_control_);
  if (flow_lower == "none")
    config.flow = config::SerialConfig::Flow::None;
  else if (flow_lower == "software")
    config.flow = config::SerialConfig::Flow::Software;
  else if (flow_lower == "hardware")
    config.flow = config::SerialConfig::Flow::Hardware;

  config.retry_interval_ms = static_cast<unsigned int>(retry_interval_.count());
  return config;
}

void Serial::set_manage_external_context(bool manage) { manage_external_context_ = manage; }

void Serial::setup_internal_handlers() {
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

void Serial::notify_state_change(base::LinkState state) {
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
