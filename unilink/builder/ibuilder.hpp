#pragma once

#include <functional>
#include <memory>

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
   * @brief Enable auto-start functionality
   * @param auto_start Whether to automatically start the wrapper
   * @return BuilderInterface& Reference to this builder for method chaining
   */
  virtual BuilderInterface& auto_start(bool auto_start = false) = 0;

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
