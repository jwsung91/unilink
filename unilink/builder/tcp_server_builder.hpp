#pragma once

#include <cstdint>

#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/tcp_server/tcp_server.hpp"

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
  TcpServerBuilder& auto_start(bool auto_start = false) override;

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
  template <typename U, typename F>
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
  template <typename U, typename F>
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
  template <typename U, typename F>
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
  template <typename U, typename F>
  TcpServerBuilder& on_error(U* obj, F method) {
    return static_cast<TcpServerBuilder&>(BuilderInterface<wrapper::TcpServer>::on_error(obj, method));
  }

  /**
   * @brief Use independent IoContext for this server (for testing isolation)
   * @param use_independent true to use independent context, false for shared context
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& use_independent_context(bool use_independent = true);

  // 멀티 클라이언트 지원 메서드
  /**
   * @brief Set multi-client connection handler callback
   * @param handler Function to handle multi-client connection events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_multi_connect(std::function<void(size_t, const std::string&)> handler);

  /**
   * @brief Set multi-client data handler callback
   * @param handler Function to handle multi-client data events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_multi_data(std::function<void(size_t, const std::string&)> handler);

  /**
   * @brief Set multi-client disconnection handler callback
   * @param handler Function to handle multi-client disconnection events
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& on_multi_disconnect(std::function<void(size_t)> handler);

  /**
   * @brief Enable port binding retry on failure
   * @param enable Whether to enable retry on port binding failure
   * @param max_retries Maximum number of retry attempts (default: 3)
   * @param retry_interval_ms Retry interval in milliseconds (default: 1000)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& enable_port_retry(bool enable = true, int max_retries = 3, int retry_interval_ms = 1000);

  /**
   * @brief Set maximum number of clients (2 or more)
   * @param max Maximum number of clients (must be 2 or more)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   * @throws std::invalid_argument if max is 0 or 1
   */
  TcpServerBuilder& max_clients(size_t max);

  /**
   * @brief Configure server for single client mode
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& single_client();

  /**
   * @brief Configure server for multi-client mode with limit
   * @param max Maximum number of clients (must be 2 or more)
   * @return TcpServerBuilder& Reference to this builder for method chaining
   * @throws std::invalid_argument if max is 0 or 1
   */
  TcpServerBuilder& multi_client(size_t max);

  /**
   * @brief Configure server for unlimited multi-client mode
   * @return TcpServerBuilder& Reference to this builder for method chaining
   */
  TcpServerBuilder& unlimited_clients();

 private:
  uint16_t port_;
  bool auto_start_;
  bool auto_manage_;
  bool use_independent_context_;

  // Port retry configuration
  bool enable_port_retry_;
  int max_port_retries_;
  int port_retry_interval_ms_;

  // Client limit configuration
  size_t max_clients_;
  bool client_limit_set_;

  std::function<void(const std::string&)> on_data_;
  std::function<void()> on_connect_;
  std::function<void()> on_disconnect_;
  std::function<void(const std::string&)> on_error_;

  // 멀티 클라이언트 콜백들
  std::function<void(size_t, const std::string&)> on_multi_connect_;
  std::function<void(size_t, const std::string&)> on_multi_data_;
  std::function<void(size_t)> on_multi_disconnect_;
};

}  // namespace builder
}  // namespace unilink
