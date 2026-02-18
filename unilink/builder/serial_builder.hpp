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

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/serial/serial.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Modernized Builder for Serial
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API SerialBuilder : public BuilderInterface<wrapper::Serial, SerialBuilder> {
 public:
  SerialBuilder(const std::string& device, uint32_t baud_rate);

  // Delete copy, allow move
  SerialBuilder(const SerialBuilder&) = delete;
  SerialBuilder& operator=(const SerialBuilder&) = delete;
  SerialBuilder(SerialBuilder&&) = default;
  SerialBuilder& operator=(SerialBuilder&&) = default;

  std::unique_ptr<wrapper::Serial> build() override;

  SerialBuilder& auto_manage(bool auto_manage = true) override;

  // Modernized event handlers
  SerialBuilder& on_data(std::function<void(const wrapper::MessageContext&)> handler) override;
  SerialBuilder& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  SerialBuilder& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) override;
  SerialBuilder& on_error(std::function<void(const wrapper::ErrorContext&)> handler) override;

  /**
   * @brief Set number of data bits
   */
  SerialBuilder& data_bits(int bits);

  /**
   * @brief Set number of stop bits
   */
  SerialBuilder& stop_bits(int bits);

  /**
   * @brief Set parity
   */
  SerialBuilder& parity(const std::string& p);

  /**
   * @brief Set flow control
   */
  SerialBuilder& flow_control(const std::string& flow);

  /**
   * @brief Set reconnection retry interval
   */
  SerialBuilder& retry_interval(uint32_t milliseconds);

  /**
   * @brief Use independent IoContext
   */
  SerialBuilder& use_independent_context(bool use_independent = true);

 private:
  std::string device_;
  uint32_t baud_rate_;
  bool auto_manage_;
  bool use_independent_context_;

  // Configuration
  int data_bits_;
  int stop_bits_;
  std::string parity_;
  std::string flow_control_;
  std::chrono::milliseconds retry_interval_;

  // Callbacks
  std::function<void(const wrapper::MessageContext&)> on_data_;
  std::function<void(const wrapper::ConnectionContext&)> on_connect_;
  std::function<void(const wrapper::ConnectionContext&)> on_disconnect_;
  std::function<void(const wrapper::ErrorContext&)> on_error_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
