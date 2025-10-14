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

#if __has_include(<unilink_export.hpp>)
#include <unilink_export.hpp>
#else
#define UNILINK_EXPORT
#endif

#include <functional>
#include <memory>
#include <string>

namespace unilink {
namespace builder {

/**
 * @brief Generic Builder interface for fluent API pattern
 *
 * This interface provides a common base for all builder classes,
 * enabling a consistent fluent API across different wrapper types.
 *
 * @tparam T The product type that this builder creates
 */
template <typename T>
class BuilderInterface {
 public:
  virtual ~BuilderInterface() = default;

  /**
   * @brief Build and return the configured product
   * @return std::unique_ptr<T> The configured wrapper instance
   */
  virtual std::unique_ptr<T> build() = 0;

  /**
   * @brief Enable auto-manage functionality
   * @param auto_manage Whether to automatically manage the wrapper lifecycle
   * @return BuilderInterface& Reference to this builder for method chaining
   */
  virtual BuilderInterface& auto_manage(bool auto_manage = true) = 0;

  /**
   * @brief Set data handler callback
   * @param handler Function to handle incoming data
   * @return BuilderInterface& Reference to this builder for method chaining
   */
  virtual BuilderInterface& on_data(std::function<void(const std::string&)> handler) = 0;

  /**
   * @brief Set data handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return IBuilder& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  BuilderInterface& on_data(U* obj, F method) {
    return on_data([obj, method](const std::string& data) { (obj->*method)(data); });
  }

  /**
   * @brief Set connection handler callback
   * @param handler Function to handle connection events
   * @return BuilderInterface& Reference to this builder for method chaining
   */
  virtual BuilderInterface& on_connect(std::function<void()> handler) = 0;

  /**
   * @brief Set connection handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return IBuilder& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  BuilderInterface& on_connect(U* obj, F method) {
    return on_connect([obj, method]() { (obj->*method)(); });
  }

  /**
   * @brief Set disconnection handler callback
   * @param handler Function to handle disconnection events
   * @return BuilderInterface& Reference to this builder for method chaining
   */
  virtual BuilderInterface& on_disconnect(std::function<void()> handler) = 0;

  /**
   * @brief Set disconnection handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return IBuilder& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  BuilderInterface& on_disconnect(U* obj, F method) {
    return on_disconnect([obj, method]() { (obj->*method)(); });
  }

  /**
   * @brief Set error handler callback
   * @param handler Function to handle error events
   * @return BuilderInterface& Reference to this builder for method chaining
   */
  virtual BuilderInterface& on_error(std::function<void(const std::string&)> handler) = 0;

  /**
   * @brief Set error handler callback using member function pointer
   * @tparam U Class type
   * @tparam F Member function type
   * @param obj Object instance
   * @param method Member function pointer
   * @return IBuilder& Reference to this builder for method chaining
   */
  template <typename U, typename F>
  BuilderInterface& on_error(U* obj, F method) {
    return on_error([obj, method](const std::string& error) { (obj->*method)(error); });
  }
};

}  // namespace builder
}  // namespace unilink
