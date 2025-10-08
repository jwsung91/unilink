#pragma once

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/tcp_client/tcp_client.hpp"
#include <cstdint>
#include <string>

namespace unilink {
namespace builder {

/**
 * @brief Builder for TcpClient wrapper
 * 
 * Provides a fluent API for configuring and creating TcpClient instances.
 * Supports method chaining for easy configuration.
 */
class TcpClientBuilder : public BuilderInterface<wrapper::TcpClient> {
public:
    /**
     * @brief Construct a new TcpClientBuilder
     * @param host The host address to connect to
     * @param port The port number to connect to
     */
    TcpClientBuilder(const std::string& host, uint16_t port);
    
    /**
     * @brief Build and return the configured TcpClient
     * @return std::unique_ptr<wrapper::TcpClient> The configured client instance
     */
    std::unique_ptr<wrapper::TcpClient> build() override;
    
    /**
     * @brief Enable auto-start functionality
     * @param auto_start Whether to automatically start the client
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& auto_start(bool auto_start = true) override;
    
    /**
     * @brief Enable auto-manage functionality
     * @param auto_manage Whether to automatically manage the client lifecycle
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& auto_manage(bool auto_manage = true) override;
    
    /**
     * @brief Set data handler callback
     * @param handler Function to handle incoming data
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& on_data(std::function<void(const std::string&)> handler) override;
    
    /**
     * @brief Set data handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpClientBuilder& on_data(U* obj, F method) {
        return static_cast<TcpClientBuilder&>(BuilderInterface<wrapper::TcpClient>::on_data(obj, method));
    }
    
    /**
     * @brief Set connection handler callback
     * @param handler Function to handle connection events
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& on_connect(std::function<void()> handler) override;
    
    /**
     * @brief Set connection handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpClientBuilder& on_connect(U* obj, F method) {
        return static_cast<TcpClientBuilder&>(BuilderInterface<wrapper::TcpClient>::on_connect(obj, method));
    }
    
    /**
     * @brief Set disconnection handler callback
     * @param handler Function to handle disconnection events
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& on_disconnect(std::function<void()> handler) override;
    
    /**
     * @brief Set disconnection handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpClientBuilder& on_disconnect(U* obj, F method) {
        return static_cast<TcpClientBuilder&>(BuilderInterface<wrapper::TcpClient>::on_disconnect(obj, method));
    }
    
    /**
     * @brief Set error handler callback
     * @param handler Function to handle error events
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& on_error(std::function<void(const std::string&)> handler) override;
    
    /**
     * @brief Set error handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpClientBuilder& on_error(U* obj, F method) {
        return static_cast<TcpClientBuilder&>(BuilderInterface<wrapper::TcpClient>::on_error(obj, method));
    }
    
    /**
     * @brief Use independent IoContext for this client (for testing isolation)
     * @param use_independent true to use independent context, false for shared context
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& use_independent_context(bool use_independent = true);
    
    /**
     * @brief Set retry interval for automatic reconnection
     * @param interval_ms Retry interval in milliseconds
     * @return TcpClientBuilder& Reference to this builder for method chaining
     */
    TcpClientBuilder& retry_interval(unsigned interval_ms);

private:
    std::string host_;
    uint16_t port_;
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

