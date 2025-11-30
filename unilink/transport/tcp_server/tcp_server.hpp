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

#include <algorithm>
#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"
#include "unilink/common/platform.hpp"
#include "unilink/common/thread_safe_state.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/interface/itcp_acceptor.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpServerConfig;
using interface::Channel;
using interface::TcpAcceptorInterface;
using tcp = net::ip::tcp;

class TcpServer : public Channel,
                  public std::enable_shared_from_this<TcpServer> {  // NOLINT
 public:
  explicit TcpServer(const TcpServerConfig& cfg);
  // Constructor for testing with dependency injection
  TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
            net::io_context& ioc);
  ~TcpServer();

  void start() override;
  void stop(std::function<void()> on_stopped = nullptr) override;
  bool is_connected() const override;
  void async_write_copy(const uint8_t* data, size_t size) override;
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Multi-client support methods
  void broadcast(const std::string& message);
  void send_to_client(size_t client_id, const std::string& message);
  size_t get_client_count() const;
  std::vector<size_t> get_connected_clients() const;

  // Multi-client callback type definitions
  using MultiClientConnectHandler = std::function<void(size_t client_id, const std::string& client_info)>;
  using MultiClientDataHandler = std::function<void(size_t client_id, const std::string& data)>;
  using MultiClientDisconnectHandler = std::function<void(size_t client_id)>;

  void on_multi_connect(MultiClientConnectHandler handler);
  void on_multi_data(MultiClientDataHandler handler);
  void on_multi_disconnect(MultiClientDisconnectHandler handler);

  // Client limit configuration
  void set_client_limit(size_t max_clients);
  void set_unlimited_clients();

 private:
  void do_accept();
  void notify_state();
  void attempt_port_binding(int retry_count);

 private:
  std::unique_ptr<net::io_context> owned_ioc_;
  bool owns_ioc_;
  net::io_context& ioc_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::TcpAcceptorInterface> acceptor_;
  TcpServerConfig cfg_;

  // Multi-client support
  std::vector<std::shared_ptr<TcpServerSession>> sessions_;
  mutable std::mutex sessions_mutex_;  // Guards sessions_ and current_session_

  // Client limit configuration
  size_t max_clients_;
  bool client_limit_enabled_;

  // Current active session for existing API compatibility
  std::shared_ptr<TcpServerSession> current_session_;

  // Multi-client callbacks
  MultiClientConnectHandler on_multi_connect_;
  MultiClientDataHandler on_multi_data_;
  MultiClientDisconnectHandler on_multi_disconnect_;

  // Existing members (compatibility maintained)
  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  ThreadSafeLinkState state_{LinkState::Idle};
};
}  // namespace transport
}  // namespace unilink
