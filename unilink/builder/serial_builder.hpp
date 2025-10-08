#pragma once

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/serial/serial.hpp"
#include <cstdint>
#include <string>

namespace unilink {
namespace builder {

/**
 * @brief Builder for Serial wrapper
 * 
 * Provides a fluent API for configuring and creating Serial instances.
 * Supports method chaining for easy configuration.
 */
class SerialBuilder : public BuilderInterface<wrapper::Serial> {
public:
    /**
     * @brief Construct a new SerialBuilder
     * @param device The serial device path (e.g., "/dev/ttyUSB0")
     * @param baud_rate The baud rate for serial communication
     */
    SerialBuilder(const std::string& device, uint32_t baud_rate);
    
    /**
     * @brief Build and return the configured Serial
     * @return std::unique_ptr<wrapper::Serial> The configured serial instance
     */
    std::unique_ptr<wrapper::Serial> build() override;
    
    /**
     * @brief Enable auto-start functionality
     * @param auto_start Whether to automatically start the serial
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    SerialBuilder& auto_start(bool auto_start = false) override;
    
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
     * @brief Set data handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    SerialBuilder& on_data(U* obj, F method) {
        return static_cast<SerialBuilder&>(BuilderInterface<wrapper::Serial>::on_data(obj, method));
    }
    
    /**
     * @brief Set connection handler callback
     * @param handler Function to handle connection events
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    SerialBuilder& on_connect(std::function<void()> handler) override;
    
    /**
     * @brief Set connection handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    SerialBuilder& on_connect(U* obj, F method) {
        return static_cast<SerialBuilder&>(BuilderInterface<wrapper::Serial>::on_connect(obj, method));
    }
    
    /**
     * @brief Set disconnection handler callback
     * @param handler Function to handle disconnection events
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    SerialBuilder& on_disconnect(std::function<void()> handler) override;
    
    /**
     * @brief Set disconnection handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    SerialBuilder& on_disconnect(U* obj, F method) {
        return static_cast<SerialBuilder&>(BuilderInterface<wrapper::Serial>::on_disconnect(obj, method));
    }
    
    /**
     * @brief Set error handler callback
     * @param handler Function to handle error events
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    SerialBuilder& on_error(std::function<void(const std::string&)> handler) override;
    
    /**
     * @brief Set error handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return SerialBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    SerialBuilder& on_error(U* obj, F method) {
        return static_cast<SerialBuilder&>(BuilderInterface<wrapper::Serial>::on_error(obj, method));
    }
    
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
    bool auto_start_;
    bool auto_manage_;
    bool use_independent_context_;
    unsigned retry_interval_ms_;
    
    std::function<void(const std::string&)> on_data_;
    std::function<void()> on_connect_;
    std::function<void()> on_disconnect_;
    std::function<void(const std::string&)> on_error_;
};

} // namespace builder
} // namespace unilink

