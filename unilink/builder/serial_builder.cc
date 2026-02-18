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

#include "unilink/builder/serial_builder.hpp"

#include <boost/asio/io_context.hpp>

namespace unilink {
namespace builder {

SerialBuilder::SerialBuilder(const std::string& device, uint32_t baud_rate)
    : device_(device),
      baud_rate_(baud_rate),
      auto_manage_(false),
      use_independent_context_(false),
      data_bits_(8),
      stop_bits_(1),
      parity_("none"),
      flow_control_("none"),
      retry_interval_(3000) {}

std::unique_ptr<wrapper::Serial> SerialBuilder::build() {
  std::unique_ptr<wrapper::Serial> serial;
  if (use_independent_context_) {
    serial = std::make_unique<wrapper::Serial>(device_, baud_rate_, std::make_shared<boost::asio::io_context>());
    serial->set_manage_external_context(true);
  } else {
    serial = std::make_unique<wrapper::Serial>(device_, baud_rate_);
  }

  if (on_data_) serial->on_data(on_data_);
  if (on_connect_) serial->on_connect(on_connect_);
  if (on_disconnect_) serial->on_disconnect(on_disconnect_);
  if (on_error_) serial->on_error(on_error_);

  serial->set_data_bits(data_bits_);
  serial->set_stop_bits(stop_bits_);
  serial->set_parity(parity_);
  serial->set_flow_control(flow_control_);
  serial->set_retry_interval(retry_interval_);

  if (auto_manage_) {
    serial->auto_manage(true);
  }

  return serial;
}

SerialBuilder& SerialBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

SerialBuilder& SerialBuilder::on_data(std::function<void(const wrapper::MessageContext&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_error(std::function<void(const wrapper::ErrorContext&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::data_bits(int bits) {
  data_bits_ = bits;
  return *this;
}

SerialBuilder& SerialBuilder::stop_bits(int bits) {
  stop_bits_ = bits;
  return *this;
}

SerialBuilder& SerialBuilder::parity(const std::string& p) {
  parity_ = p;
  return *this;
}

SerialBuilder& SerialBuilder::flow_control(const std::string& flow) {
  flow_control_ = flow;
  return *this;
}

SerialBuilder& SerialBuilder::retry_interval(uint32_t milliseconds) {
  retry_interval_ = std::chrono::milliseconds(milliseconds);
  return *this;
}

SerialBuilder& SerialBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

}  // namespace builder
}  // namespace unilink
