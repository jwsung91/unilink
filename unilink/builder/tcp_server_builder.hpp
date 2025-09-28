#pragma once

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include <cstdint>

namespace unilink {
namespace builder {

/**
 * @brief Builder for TcpServer wrapper
 * 
 * Provides a fluent API for configuring and creating TcpServer instances.
 * Supports method chaining for easy configuration.
 */
class TcpServerBuilder : public BuilderInterface<wrapper::TcpServer> {
public:
    /**
     * @brief Construct a new TcpServerBuilder
     * @param port The port number for the server
     */
    explicit TcpServerBuilder(uint16_t port);
    
    /**
     * @brief Build and return the configured TcpServer
     * @return std::unique_ptr<wrapper::TcpServer> The configured server instance
     */
    std::unique_ptr<wrapper::TcpServer> build() override;
    
    /**
     * @brief Enable auto-start functionality
     * @param auto_start Whether to automatically start the server
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    TcpServerBuilder& auto_start(bool auto_start = true) override;
    
    /**
     * @brief Enable auto-manage functionality
     * @param auto_manage Whether to automatically manage the server lifecycle
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    TcpServerBuilder& auto_manage(bool auto_manage = true) override;
    
    /**
     * @brief Set data handler callback
     * @param handler Function to handle incoming data
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    TcpServerBuilder& on_data(std::function<void(const std::string&)> handler) override;
    
    /**
     * @brief Set data handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpServerBuilder& on_data(U* obj, F method) {
        return static_cast<TcpServerBuilder&>(BuilderInterface<wrapper::TcpServer>::on_data(obj, method));
    }
    
    /**
     * @brief Set connection handler callback
     * @param handler Function to handle connection events
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    TcpServerBuilder& on_connect(std::function<void()> handler) override;
    
    /**
     * @brief Set connection handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpServerBuilder& on_connect(U* obj, F method) {
        return static_cast<TcpServerBuilder&>(BuilderInterface<wrapper::TcpServer>::on_connect(obj, method));
    }
    
    /**
     * @brief Set disconnection handler callback
     * @param handler Function to handle disconnection events
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    TcpServerBuilder& on_disconnect(std::function<void()> handler) override;
    
    /**
     * @brief Set disconnection handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpServerBuilder& on_disconnect(U* obj, F method) {
        return static_cast<TcpServerBuilder&>(BuilderInterface<wrapper::TcpServer>::on_disconnect(obj, method));
    }
    
    /**
     * @brief Set error handler callback
     * @param handler Function to handle error events
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    TcpServerBuilder& on_error(std::function<void(const std::string&)> handler) override;
    
    /**
     * @brief Set error handler callback using member function pointer
     * @tparam U Class type
     * @tparam F Member function type
     * @param obj Object instance
     * @param method Member function pointer
     * @return TcpServerBuilder& Reference to this builder for method chaining
     */
    template<typename U, typename F>
    TcpServerBuilder& on_error(U* obj, F method) {
        return static_cast<TcpServerBuilder&>(BuilderInterface<wrapper::TcpServer>::on_error(obj, method));
    }

private:
    uint16_t port_;
    bool auto_start_;
    bool auto_manage_;
    
    std::function<void(const std::string&)> on_data_;
    std::function<void()> on_connect_;
    std::function<void()> on_disconnect_;
    std::function<void(const std::string&)> on_error_;
};

} // namespace builder
} // namespace unilink

