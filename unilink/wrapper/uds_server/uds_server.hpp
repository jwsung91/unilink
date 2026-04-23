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

#include <chrono>
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
 * @brief Modernized UDS Server Wrapper
 */
class UNILINK_API UdsServer : public ServerInterface {
 public:
  explicit UdsServer(const std::string& socket_path);
  UdsServer(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> external_ioc);
  explicit UdsServer(std::shared_ptr<interface::Channel> channel);
  ~UdsServer() override;

  // Move semantics
  UdsServer(UdsServer&&) noexcept;
  UdsServer& operator=(UdsServer&&) noexcept;

  // Disable copy
  UdsServer(const UdsServer&) = delete;
  UdsServer& operator=(const UdsServer&) = delete;

  // ServerInterface implementation
  [[nodiscard]] std::future<bool> start() override;
  void stop() override;
  bool listening() const override;

  // Transmission
  bool broadcast(std::string_view data) override;
  bool send_to(ClientId client_id, std::string_view data) override;

  // Event handlers
  ServerInterface& on_connect(ConnectionHandler handler) override;
  ServerInterface& on_disconnect(ConnectionHandler handler) override;
  ServerInterface& on_data(MessageHandler handler) override;
  ServerInterface& on_error(ErrorHandler handler) override;

  ServerInterface& framer(FramerFactory factory) override;
  ServerInterface& on_message(MessageHandler handler) override;

  // Client count and management
  size_t client_count() const override;
  std::vector<ClientId> connected_clients() const override;

  // Configuration (Fluent API)
  UdsServer& auto_start(bool manage = true) override;
  UdsServer& idle_timeout(std::chrono::milliseconds timeout);
  UdsServer& max_clients(size_t max);
  UdsServer& unlimited_clients();
  UdsServer& manage_external_context(bool manage);

 private:
  struct Impl;
  const Impl* get_impl() const { return impl_.get(); }
  Impl* get_impl() { return impl_.get(); }
  std::unique_ptr<Impl> impl_;
};

}  // namespace wrapper
}  // namespace unilink
