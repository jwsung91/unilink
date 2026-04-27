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

#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/base/visibility.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/wrapper/iserver.hpp"

namespace unilink {
namespace interface {
class Channel;
}

namespace wrapper {

/**
 * @brief UDP implementation of ServerInterface using virtual sessions.
 */
class UNILINK_API UdpServer : public ServerInterface {
 public:
  explicit UdpServer(uint16_t port);
  explicit UdpServer(const config::UdpConfig& cfg);
  UdpServer(const config::UdpConfig& cfg, std::shared_ptr<boost::asio::io_context> external_ioc);
  explicit UdpServer(std::shared_ptr<interface::Channel> channel);
  ~UdpServer() override;

  UdpServer(UdpServer&&) noexcept;
  UdpServer& operator=(UdpServer&&) noexcept;

  UdpServer(const UdpServer&) = delete;
  UdpServer& operator=(const UdpServer&) = delete;

  // Lifecycle
  // ServerInterface implementation
  [[nodiscard]] std::future<bool> start() override;
  void stop() override;
  bool listening() const override;

  // Transmission
  bool broadcast(std::string_view data) override;
  bool send_to(ClientId client_id, std::string_view data) override;
  bool send_to_blocking(ClientId client_id, std::string_view data) override;
  bool try_send_to(ClientId client_id, std::string_view data) override;
  bool try_broadcast(std::string_view data) override;

  // Event handlers
  ServerInterface& on_connect(ConnectionHandler handler) override;
  ServerInterface& on_disconnect(ConnectionHandler handler) override;
  ServerInterface& on_data(MessageHandler handler) override;
  ServerInterface& on_data_batch(BatchMessageHandler handler) override;
  ServerInterface& on_error(ErrorHandler handler) override;

  ServerInterface& framer(FramerFactory factory) override;
  ServerInterface& on_message(MessageHandler handler) override;
  ServerInterface& on_message_batch(BatchMessageHandler handler) override;

  // Client count and management
  size_t client_count() const override;
  std::vector<ClientId> connected_clients() const override;

  // UDP specific
  UdpServer& auto_start(bool manage = true) override;
  UdpServer& session_timeout(std::chrono::milliseconds timeout);
  UdpServer& on_backpressure(std::function<void(size_t)> handler);
  UdpServer& backpressure_threshold(size_t threshold);
  UdpServer& backpressure_strategy(base::constants::BackpressureStrategy strategy);
  UdpServer& manage_external_context(bool manage);

 private:
  struct Impl;
  const Impl* get_impl() const { return impl_.get(); }
  Impl* get_impl() { return impl_.get(); }
  std::unique_ptr<Impl> impl_;
};

}  // namespace wrapper
}  // namespace unilink
