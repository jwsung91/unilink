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

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/framer/iframer.hpp"
#include "unilink/framer/line_framer.hpp"
#include "unilink/framer/packet_framer.hpp"
#include "unilink/memory/safe_span.hpp"
#include "unilink/wrapper/context.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Generic Builder interface for fluent API pattern
 *
 * This interface provides a common base for all builder classes,
 * enabling a consistent fluent API across different wrapper types.
 *
 * @tparam T The product type that this builder creates
 * @tparam Derived The derived builder type (CRTP)
 */
template <typename T, typename Derived>
class BuilderInterface {
 public:
  virtual ~BuilderInterface() = default;
  BuilderInterface() = default;
  BuilderInterface(const BuilderInterface&) = default;
  BuilderInterface(BuilderInterface&&) = default;
  BuilderInterface& operator=(const BuilderInterface&) = default;
  BuilderInterface& operator=(BuilderInterface&&) = default;

  /**
   * @brief Build and return the configured product
   * @return std::unique_ptr<T> The configured wrapper instance
   */
  virtual std::unique_ptr<T> build() = 0;

  /**
   * @brief Enable auto-manage functionality
   * @param auto_manage Whether to automatically manage the wrapper lifecycle
   * @return Derived& Reference to this builder for method chaining
   */
  virtual Derived& auto_manage(bool auto_manage = true) = 0;

  /**
   * @brief Set data handler callback
   * @param handler Function to handle incoming data with context
   * @return Derived& Reference to this builder for method chaining
   */
  virtual Derived& on_data(std::function<void(const wrapper::MessageContext&)> handler) = 0;

  /**
   * @brief Set data handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return Derived& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  Derived& on_data(U* obj, F method) {
    return on_data([obj, method](const wrapper::MessageContext& ctx) { (obj->*method)(ctx); });
  }

  /**
   * @brief Set connection handler callback
   * @param handler Function to handle connection events with context
   * @return Derived& Reference to this builder for method chaining
   */
  virtual Derived& on_connect(std::function<void(const wrapper::ConnectionContext&)> handler) = 0;

  /**
   * @brief Set connection handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return Derived& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  Derived& on_connect(U* obj, F method) {
    return on_connect([obj, method](const wrapper::ConnectionContext& ctx) { (obj->*method)(ctx); });
  }

  /**
   * @brief Set disconnection handler callback
   * @param handler Function to handle disconnection events with context
   * @return Derived& Reference to this builder for method chaining
   */
  virtual Derived& on_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) = 0;

  /**
   * @brief Set disconnection handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return Derived& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  Derived& on_disconnect(U* obj, F method) {
    return on_disconnect([obj, method](const wrapper::ConnectionContext& ctx) { (obj->*method)(ctx); });
  }

  /**
   * @brief Set error handler callback
   * @param handler Function to handle error events with context
   * @return Derived& Reference to this builder for method chaining
   */
  virtual Derived& on_error(std::function<void(const wrapper::ErrorContext&)> handler) = 0;

  /**
   * @brief Set error handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return Derived& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  Derived& on_error(U* obj, F method) {
    return on_error([obj, method](const wrapper::ErrorContext& ctx) { (obj->*method)(ctx); });
  }

  // Framing Support (remains focused on raw data for now, can be evolved)
  
  /**
   * @brief Use LineFramer for message segmentation (e.g., newline delimited)
   * @param delimiter Delimiter string (default: "\n")
   * @param include_delimiter Whether to include delimiter in the message
   * @param max_length Maximum message length
   * @return Derived& Reference to this builder
   */
  Derived& use_line_framer(std::string_view delimiter = "\n", bool include_delimiter = false,
                           size_t max_length = 65536) {
    framer_ = std::make_unique<framer::LineFramer>(delimiter, include_delimiter, max_length);
    return static_cast<Derived&>(*this);
  }

  /**
   * @brief Use PacketFramer for message segmentation (binary pattern matching)
   * @param start_pattern Start pattern bytes
   * @param end_pattern End pattern bytes
   * @param max_length Maximum packet length
   * @return Derived& Reference to this builder
   */
  Derived& use_packet_framer(const std::vector<uint8_t>& start_pattern, const std::vector<uint8_t>& end_pattern,
                             size_t max_length) {
    framer_ = std::make_unique<framer::PacketFramer>(start_pattern, end_pattern, max_length);
    return static_cast<Derived&>(*this);
  }

  /**
   * @brief Set message handler callback (for framed messages)
   * @param handler Function to handle complete messages
   * @return Derived& Reference to this builder
   */
  Derived& on_message(std::function<void(memory::ConstByteSpan)> handler) {
    on_message_ = std::move(handler);
    return static_cast<Derived&>(*this);
  }

  /**
   * @brief Set message handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return Derived& Reference to this builder
   */
  template <typename U, typename F>
  Derived& on_message(U* obj, F method) {
    return on_message([obj, method](memory::ConstByteSpan msg) { (obj->*method)(msg); });
  }

 protected:
  std::unique_ptr<framer::IFramer> framer_;
  std::function<void(memory::ConstByteSpan)> on_message_;
};

}  // namespace builder
}  // namespace unilink
