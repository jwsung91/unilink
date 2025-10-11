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
  // Channel은 나중에 start() 시점에 생성
}

Serial::Serial(std::shared_ptr<interface::Channel> channel) : device_(""), baud_rate_(9600), channel_(channel) {
  setup_internal_handlers();
}

Serial::~Serial() {
  // 강제로 정리 - auto_manage 설정과 관계없이
  if (started_) {
    stop();
  }
  // Channel 리소스 명시적 정리
  if (channel_) {
    channel_.reset();
  }
}

void Serial::start() {
  if (started_) return;

  if (!channel_) {
    // Channel 생성
    config::SerialConfig config;
    config.device = device_;
    config.baud_rate = baud_rate_;
    config.char_size = data_bits_;
    config.stop_bits = stop_bits_;
    config.retry_interval_ms = static_cast<unsigned int>(retry_interval_.count());
    // parity와 flow는 enum으로 변환 필요
    config.flow = unilink::config::SerialConfig::Flow::None;
    channel_ = factory::ChannelFactory::create(config);
    setup_internal_handlers();
  }

  channel_->start();
  started_ = true;
}

void Serial::stop() {
  if (!started_ || !channel_) return;

  channel_->stop();
  // 잠시 대기하여 비동기 작업 완료
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  channel_.reset();
  started_ = false;
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

  // 바이트 데이터를 문자열로 변환하여 전달
  channel_->on_bytes([this](const uint8_t* p, size_t n) {
    if (data_handler_) {
      std::string data = common::safe_convert::uint8_to_string(p, n);
      data_handler_(data);
    }
  });

  // 상태 변화 처리
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

}  // namespace wrapper
}  // namespace unilink
