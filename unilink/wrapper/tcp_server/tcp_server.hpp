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

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "unilink/common/visibility.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/wrapper/ichannel.hpp"

namespace unilink {
namespace wrapper {

/**
 * Improved TCP Server Wrapper
 * - Uses shared io_context
 * - Prevents memory leaks
 * - Automatic resource management
 */
class UNILINK_API TcpServer : public ChannelInterface {
 public:
  explicit TcpServer(uint16_t port);
  TcpServer(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc);
  explicit TcpServer(std::shared_ptr<interface::Channel> channel);
  ~TcpServer();

  // IChannel interface implementation
  void start() override;
  void stop() override;
  void send(const std::string& data) override;
  bool is_connected() const override;

  ChannelInterface& on_data(DataHandler handler) override;
  ChannelInterface& on_connect(ConnectHandler handler) override;
  ChannelInterface& on_disconnect(DisconnectHandler handler) override;
  ChannelInterface& on_error(ErrorHandler handler) override;

  ChannelInterface& auto_manage(bool manage = true) override;

  void send_line(const std::string& line) override;

  // Multi-client support methods
  bool broadcast(const std::string& message);
  bool send_to_client(size_t client_id, const std::string& message);
  size_t get_client_count() const;
  std::vector<size_t> get_connected_clients() const;

  // Multi-client callback type definitions
  using MultiClientConnectHandler = std::function<void(size_t client_id, const std::string& client_info)>;
  using MultiClientDataHandler = std::function<void(size_t client_id, const std::string& data)>;
  using MultiClientDisconnectHandler = std::function<void(size_t client_id)>;

  TcpServer& on_multi_connect(MultiClientConnectHandler handler);
  TcpServer& on_multi_data(MultiClientDataHandler handler);
  TcpServer& on_multi_disconnect(MultiClientDisconnectHandler handler);

  // 실패 시 bool 반환 외에 on_error 알림을 받을지 설정
  TcpServer& notify_send_failure(bool enable = true);

  // Client limit configuration
  void set_client_limit(size_t max_clients);
  void set_unlimited_clients();

  // Port retry configuration
  TcpServer& enable_port_retry(bool enable = true, int max_retries = 3, int retry_interval_ms = 1000);

  // Server state checking
  bool is_listening() const;
  void set_manage_external_context(bool manage);

 private:
  void setup_internal_handlers();
  void handle_bytes(const uint8_t* data, size_t size);
  void handle_state(common::LinkState state);

  mutable std::mutex mutex_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;
  bool started_{false};
  bool auto_manage_{false};
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;

  // Port retry configuration
  bool port_retry_enabled_{false};
  int max_port_retries_{3};
  int port_retry_interval_ms_{1000};

  // Client limit configuration
  bool client_limit_enabled_{false};
  size_t max_clients_{0};

  // Server state tracking
  std::atomic<bool> is_listening_{false};

  // User callbacks
  DataHandler on_data_;
  ConnectHandler on_connect_;
  DisconnectHandler on_disconnect_;
  ErrorHandler on_error_;

  // Multi-client callbacks
  MultiClientConnectHandler on_multi_connect_;
  MultiClientDataHandler on_multi_data_;
  MultiClientDisconnectHandler on_multi_disconnect_;

  // Send/broadcast 실패 시 on_error 호출 여부
  bool notify_send_failure_{false};
};

}  // namespace wrapper
}  // namespace unilink
