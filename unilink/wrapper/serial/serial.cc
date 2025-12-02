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
#include <chrono>
#include <iostream>
#include <thread>

#include "unilink/config/serial_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/serial/serial.hpp"

namespace unilink {
namespace wrapper {

Serial::Serial(const std::string& device, uint32_t baud_rate)
    : device_(device), baud_rate_(baud_rate), channel_(nullptr) {
  // Channel will be created later at start() time
}

Serial::Serial(std::shared_ptr<interface::Channel> channel) : device_(""), baud_rate_(9600), channel_(channel) {
  setup_internal_handlers();
}

Serial::~Serial() {
  // Force cleanup - regardless of auto_manage setting
  if (started_) {
    stop();
  }
  // Explicit cleanup of Channel resources
  if (channel_) {
    channel_.reset();
  }
}

void Serial::start() {
  if (started_) return;

  // Ensure flow/parity mapping is applied on subsequent starts if channel was recreated
  if (!channel_) {
    auto config = build_config();
    channel_ = factory::ChannelFactory::create(config);
    setup_internal_handlers();
  } else {
    // Channel already exists; nothing to do if already started
    if (started_) return;
  }

  channel_->start();
  started_ = true;
}

void Serial::stop(std::function<void()> on_stopped) {
  std::shared_ptr<interface::Channel> local_channel;
  {
    if (!started_ || !channel_) {
      if (on_stopped) {
        on_stopped();
      }
      return;
    }
    started_ = false;
    local_channel = std::move(channel_);
  }

  if (local_channel) {
    local_channel->on_state(nullptr);
    local_channel->on_bytes(nullptr);
    local_channel->stop(on_stopped);
  } else {
    if (on_stopped) {
      on_stopped();
    }
  }
}

void Serial::send(const std::string& data) {
  if (is_connected() && channel_) {
    auto binary_data = common::safe_convert::string_to_uint8(data);
    channel_->async_write_copy(binary_data.data(), binary_data.size());
  }
}

void Serial::send_line(const std::string& line) { send(line + "\n"); }

bool Serial::is_connected() const { return channel_ && channel_->is_connected(); }

ChannelInterface& Serial::on_data(DataHandler handler) {
  data_handler_ = std::move(handler);
  if (channel_) {
    setup_internal_handlers();
  }
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
  return *this;
}

void Serial::set_baud_rate(uint32_t baud_rate) { baud_rate_ = baud_rate; }

void Serial::set_data_bits(int data_bits) { data_bits_ = data_bits; }

void Serial::set_stop_bits(int stop_bits) { stop_bits_ = stop_bits; }

void Serial::set_parity(const std::string& parity) { parity_ = parity; }

void Serial::set_flow_control(const std::string& flow_control) { flow_control_ = flow_control; }

void Serial::setup_internal_handlers() {
  if (!channel_) return;

  // Convert byte data to string and pass it
  channel_->on_bytes([this](const uint8_t* p, size_t n) {
    if (data_handler_) {
      std::string data = common::safe_convert::uint8_to_string(p, n);
      data_handler_(data);
    }
  });

  // Handle state changes
  channel_->on_state([this](common::LinkState state) { notify_state_change(state); });
}

void Serial::notify_state_change(common::LinkState state) {
  switch (state) {
    case common::LinkState::Connecting:
      // Connecting state - no action needed, just log
      break;
    case common::LinkState::Connected:
      if (connect_handler_) connect_handler_();
      break;
    case common::LinkState::Closed:
      if (disconnect_handler_) disconnect_handler_();
      break;
    case common::LinkState::Error:
      if (error_handler_) error_handler_("Serial connection error occurred");
      break;
    default:
      break;
  }
}

void Serial::set_retry_interval(std::chrono::milliseconds interval) {
  retry_interval_ = interval;

  // If channel is already created, update its retry interval
  if (channel_) {
    // Cast to transport::Serial and set retry interval
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
  config.char_size = static_cast<unsigned>(std::clamp(data_bits_, 5, 8));
  config.stop_bits = static_cast<unsigned>(std::clamp(stop_bits_, 1, 2));
  config.retry_interval_ms = static_cast<unsigned int>(retry_interval_.count());

  std::string parity_lower = parity_;
  std::transform(parity_lower.begin(), parity_lower.end(), parity_lower.begin(), ::tolower);
  if (parity_lower == "even") {
    config.parity = config::SerialConfig::Parity::Even;
  } else if (parity_lower == "odd") {
    config.parity = config::SerialConfig::Parity::Odd;
  } else {
    config.parity = config::SerialConfig::Parity::None;
  }

  std::string flow_lower = flow_control_;
  std::transform(flow_lower.begin(), flow_lower.end(), flow_lower.begin(), ::tolower);
  if (flow_lower == "software" || flow_lower == "xonxoff" || flow_lower == "soft") {
    config.flow = config::SerialConfig::Flow::Software;
  } else if (flow_lower == "hardware" || flow_lower == "rtscts" || flow_lower == "hard") {
    config.flow = config::SerialConfig::Flow::Hardware;
  } else {
    config.flow = config::SerialConfig::Flow::None;
  }

  return config;
}

}  // namespace wrapper
}  // namespace unilink
