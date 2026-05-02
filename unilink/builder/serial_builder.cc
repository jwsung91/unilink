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

#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <cctype>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/diagnostics/exceptions.hpp"

namespace unilink {
namespace builder {

template <uint32_t State>
SerialBuilder<State>::SerialBuilder(const std::string& device, uint32_t baud_rate)
    : device_(device),
      baud_rate_(baud_rate),
      auto_start_(false),
      independent_context_(false),
      retry_interval_ms_(3000),
      char_size_(8),
      stop_bits_(1),
      parity_(config::SerialConfig::Parity::None),
      flow_(config::SerialConfig::Flow::None),
      reopen_on_error_(true) {
  if (device.empty()) throw diagnostics::BuilderException("Device path cannot be empty");

  // Ensure background IO service is running
  AutoInitializer::ensure_io_context_running();
}

template <uint32_t State>
std::unique_ptr<wrapper::Serial> SerialBuilder<State>::build() {
#if __cplusplus >= 202002L
  if constexpr (!((State & BuilderState::Ready) == BuilderState::Ready)) {
    throw diagnostics::BuilderException("SerialBuilder: Mandatory handlers (on_data and on_error) must be set.");
  }
#endif

  std::unique_ptr<wrapper::Serial> serial;
  if (independent_context_) {
    serial = std::make_unique<wrapper::Serial>(device_, baud_rate_, std::make_shared<boost::asio::io_context>());
    serial->manage_external_context(true);
  } else {
    serial = std::make_unique<wrapper::Serial>(device_, baud_rate_);
  }

  if (this->on_data_) serial->on_data(this->on_data_);
  if (this->on_connect_) serial->on_connect(this->on_connect_);
  if (this->on_disconnect_) serial->on_disconnect(this->on_disconnect_);
  if (this->on_error_) serial->on_error(this->on_error_);

  // Note: wrapper::Serial setters use strings for enum types
  std::string p_str = "none";
  if (parity_ == config::SerialConfig::Parity::Even)
    p_str = "even";
  else if (parity_ == config::SerialConfig::Parity::Odd)
    p_str = "odd";
  serial->parity(p_str);

  std::string f_str = "none";
  if (flow_ == config::SerialConfig::Flow::Software)
    f_str = "software";
  else if (flow_ == config::SerialConfig::Flow::Hardware)
    f_str = "hardware";
  serial->flow_control(f_str);

  serial->retry_interval(std::chrono::milliseconds(retry_interval_ms_));

  if (this->bp_strategy_set_) serial->backpressure_strategy(this->bp_strategy_);
  serial->backpressure_threshold(this->get_effective_backpressure_threshold());

  if (this->framer_factory_) {
    serial->framer(this->framer_factory_());
  }
  if (this->on_message_) {
    serial->on_message(std::move(this->on_message_));
  }

  if (auto_start_) {
    serial->auto_start(true);
  }

  return serial;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::char_size(unsigned int size) {
  char_size_ = size;
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::stop_bits(unsigned int bits) {
  stop_bits_ = bits;
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::parity(config::SerialConfig::Parity p) {
  parity_ = p;
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::parity(const std::string& p) {
  std::string value = p;
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (value == "even") {
    parity_ = config::SerialConfig::Parity::Even;
  } else if (value == "odd") {
    parity_ = config::SerialConfig::Parity::Odd;
  } else {
    parity_ = config::SerialConfig::Parity::None;
  }
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::flow_control(config::SerialConfig::Flow f) {
  flow_ = f;
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::flow_control(const std::string& f) {
  std::string value = f;
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (value == "software") {
    flow_ = config::SerialConfig::Flow::Software;
  } else if (value == "hardware") {
    flow_ = config::SerialConfig::Flow::Hardware;
  } else {
    flow_ = config::SerialConfig::Flow::None;
  }
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::reopen_on_error(bool enable) {
  reopen_on_error_ = enable;
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::retry_interval(std::chrono::milliseconds interval) {
  retry_interval_ms_ = static_cast<uint32_t>(interval.count());
  return *this;
}

template <uint32_t State>
SerialBuilder<State>& SerialBuilder<State>::independent_context(bool use_independent) {
  independent_context_ = use_independent;
  return *this;
}

// Explicit template instantiations
template class SerialBuilder<BuilderState::None>;
template class SerialBuilder<BuilderState::HasData>;
template class SerialBuilder<BuilderState::HasError>;
template class SerialBuilder<BuilderState::Ready>;

}  // namespace builder
}  // namespace unilink
