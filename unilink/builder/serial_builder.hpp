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

#include <cstdint>
#include <string>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/serial/serial.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Builder for Serial wrapper
 *
 * Provides a fluent API for configuring and creating Serial instances.
 * Supports method chaining for easy configuration.
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API SerialBuilder : public BuilderInterface<wrapper::Serial, SerialBuilder> {
 public:
  /**
   * @brief Construct a new SerialBuilder
   * @param device The serial device path (e.g., "/dev/ttyUSB0")
   * @param baud_rate The baud rate for serial communication
   */
  SerialBuilder(const std::string& device, uint32_t baud_rate);

  // Delete copy, allow move
  SerialBuilder(const SerialBuilder&) = delete;
  SerialBuilder& operator=(const SerialBuilder&) = delete;
  SerialBuilder(SerialBuilder&&) = default;
  SerialBuilder& operator=(SerialBuilder&&) = default;

  using BuilderInterface::on_connect;
  using BuilderInterface::on_data;
  using BuilderInterface::on_disconnect;
  using BuilderInterface::on_error;

  /**
   * @brief Build and return the configured Serial
   * @return std::unique_ptr<wrapper::Serial> The configured serial instance
   */
  std::unique_ptr<wrapper::Serial> build() override;

  /**
   * @brief Enable auto-manage functionality
   * @param auto_manage Whether to automatically manage the serial lifecycle
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& auto_manage(bool auto_manage = true) override;

  /**
   * @brief Set data handler callback
   * @param handler Function to handle incoming data
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& on_data(std::function<void(const std::string&)> handler) override;

  /**
   * @brief Set connection handler callback
   * @param handler Function to handle connection events
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& on_connect(std::function<void()> handler) override;

  /**
   * @brief Set disconnection handler callback
   * @param handler Function to handle disconnection events
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& on_disconnect(std::function<void()> handler) override;

  /**
   * @brief Set error handler callback
   * @param handler Function to handle error events
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& on_error(std::function<void(const std::string&)> handler) override;

  /**
   * @brief Use independent IoContext for this serial (for testing isolation)
   * @param use_independent true to use independent context, false for shared context
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& use_independent_context(bool use_independent = true);

  /**
   * @brief Set retry interval for automatic reconnection
   * @param interval_ms Retry interval in milliseconds
   * @return SerialBuilder& Reference to this builder for method chaining
   */
  SerialBuilder& retry_interval(unsigned interval_ms);

 private:
  std::string device_;
  uint32_t baud_rate_;
  bool auto_manage_;
  bool use_independent_context_;
  unsigned retry_interval_ms_;

  std::function<void(const std::string&)> on_data_;
  std::function<void()> on_connect_;
  std::function<void()> on_disconnect_;
  std::function<void(const std::string&)> on_error_;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink
