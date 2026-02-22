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
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/wrapper/iserver.hpp"

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace unilink {

namespace interface {
class Channel;
}

namespace wrapper {

/**
 * @brief Modernized TCP Server Wrapper
 */
class UNILINK_API TcpServer : public ServerInterface {
 public:
  explicit TcpServer(uint16_t port);
  TcpServer(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc);
  explicit TcpServer(std::shared_ptr<interface::Channel> channel);
  ~TcpServer() override;

  // Move semantics
  TcpServer(TcpServer&&) noexcept;
  TcpServer& operator=(TcpServer&&) noexcept;

  // Disable copy
  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;

  // ServerInterface implementation
  std::future<bool> start() override;
  void stop() override;
  bool is_listening() const override;

  // Transmission
  bool broadcast(std::string_view data) override;
  bool send_to(size_t client_id, std::string_view data) override;

  // Event handlers
  ServerInterface& on_client_connect(ConnectionHandler handler) override;
  ServerInterface& on_client_disconnect(ConnectionHandler handler) override;
  ServerInterface& on_data(MessageHandler handler) override;
  ServerInterface& on_error(ErrorHandler handler) override;

  // Client count and management
  size_t get_client_count() const override;
  std::vector<size_t> get_connected_clients() const override;

  // Configuration (Fluent API)
  TcpServer& auto_manage(bool manage = true);
  TcpServer& bind_address(const std::string& address);
  TcpServer& enable_port_retry(bool enable = true, int max_retries = 3, int retry_interval_ms = 1000);
  TcpServer& idle_timeout(int timeout_ms);
  TcpServer& set_client_limit(size_t max_clients);
  TcpServer& set_unlimited_clients();
  TcpServer& notify_send_failure(bool enable = true);
  TcpServer& set_manage_external_context(bool manage);

 private:
  struct Impl;
  const Impl* get_impl() const { return impl_.get(); }
  Impl* get_impl() { return impl_.get(); }
  std::unique_ptr<Impl> impl_;
};

}  // namespace wrapper
}  // namespace unilink
