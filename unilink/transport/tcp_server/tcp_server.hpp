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

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/memory/memory_pool.hpp"

// Forward declarations to avoid Boost dependency in header
namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace unilink {
namespace interface {
class TcpAcceptorInterface;
}
namespace transport {

using base::LinkState;
using config::TcpServerConfig;
using interface::Channel;

class UNILINK_API TcpServer : public Channel,
                              public std::enable_shared_from_this<TcpServer> {  // NOLINT
 public:
  static std::shared_ptr<TcpServer> create(const TcpServerConfig& cfg);
  static std::shared_ptr<TcpServer> create(const TcpServerConfig& cfg,
                                           std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                                           boost::asio::io_context& ioc);
  ~TcpServer() override;

  void start() override;
  void stop() override;
  bool is_connected() const override;
  void async_write_copy(memory::ConstByteSpan data) override;
  void async_write_move(std::vector<uint8_t>&& data) override;
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override;
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Multi-client support methods
  bool broadcast(std::string_view message);
  bool send_to_client(size_t client_id, std::string_view message);
  size_t get_client_count() const;
  std::vector<size_t> get_connected_clients() const;

  // Async stop request (safe to call from callbacks)
  void request_stop();

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

  // Public getter for state for testing
  base::LinkState get_state() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // Private constructors
  explicit TcpServer(const TcpServerConfig& cfg);
  TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
            boost::asio::io_context& ioc);
};

}  // namespace transport
}  // namespace unilink
