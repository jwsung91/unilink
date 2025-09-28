#pragma once

#include <memory>
#include <functional>

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
template<typename T>
class IBuilder {
public:
    virtual ~IBuilder() = default;
    
    /**
     * @brief Build and return the configured product
     * @return std::unique_ptr<T> The configured wrapper instance
     */
    virtual std::unique_ptr<T> build() = 0;
    
    /**
     * @brief Enable auto-start functionality
     * @param auto_start Whether to automatically start the wrapper
     * @return IBuilder& Reference to this builder for method chaining
     */
    virtual IBuilder& auto_start(bool auto_start = true) = 0;
    
    /**
     * @brief Enable auto-manage functionality
     * @param auto_manage Whether to automatically manage the wrapper lifecycle
     * @return IBuilder& Reference to this builder for method chaining
     */
    virtual IBuilder& auto_manage(bool auto_manage = true) = 0;
    
    /**
     * @brief Set data handler callback
     * @param handler Function to handle incoming data
     * @return IBuilder& Reference to this builder for method chaining
     */
    virtual IBuilder& on_data(std::function<void(const std::string&)> handler) = 0;
    
    /**
     * @brief Set connection handler callback
     * @param handler Function to handle connection events
     * @return IBuilder& Reference to this builder for method chaining
     */
    virtual IBuilder& on_connect(std::function<void()> handler) = 0;
    
    /**
     * @brief Set disconnection handler callback
     * @param handler Function to handle disconnection events
     * @return IBuilder& Reference to this builder for method chaining
     */
    virtual IBuilder& on_disconnect(std::function<void()> handler) = 0;
    
    /**
     * @brief Set error handler callback
     * @param handler Function to handle error events
     * @return IBuilder& Reference to this builder for method chaining
     */
    virtual IBuilder& on_error(std::function<void(const std::string&)> handler) = 0;
};

} // namespace builder
} // namespace unilink

