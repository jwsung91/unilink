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
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/wrapper/ichannel.hpp"

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
  ~TcpServer() override;

  // IChannel interface implementation
  void start() override;
  void stop() override;
  void send(std::string_view data) override;
  bool is_connected() const override;

  ChannelInterface& on_data(DataHandler handler) override;
  ChannelInterface& on_bytes(BytesHandler handler) override;
  ChannelInterface& on_connect(ConnectHandler handler) override;
  ChannelInterface& on_disconnect(DisconnectHandler handler) override;
  ChannelInterface& on_error(ErrorHandler handler) override;

  ChannelInterface& auto_manage(bool manage = true) override;

  void send_line(std::string_view line) override;

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

  // Idle timeout configuration
  TcpServer& idle_timeout(int timeout_ms);

  // Server state checking
  bool is_listening() const;
  void set_manage_external_context(bool manage);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace wrapper
}  // namespace unilink
