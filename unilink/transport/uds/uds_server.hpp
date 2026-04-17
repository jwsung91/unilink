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

#include <memory>
#include <string>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/memory/safe_span.hpp"

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace unilink {

namespace interface {
class UdsAcceptorInterface;
}

namespace transport {

/**
 * @brief Thread-safe UDS Server implementation
 */
class UNILINK_API UdsServer : public interface::Channel, public std::enable_shared_from_this<UdsServer> {
 public:
  static std::shared_ptr<UdsServer> create(const config::UdsServerConfig& cfg);
  static std::shared_ptr<UdsServer> create(const config::UdsServerConfig& cfg,
                                           std::unique_ptr<interface::UdsAcceptorInterface> acceptor,
                                           boost::asio::io_context& ioc);
  ~UdsServer() override;

  // Move semantics
  UdsServer(UdsServer&&) noexcept;
  UdsServer& operator=(UdsServer&&) noexcept;

  // Non-copyable
  UdsServer(const UdsServer&) = delete;
  UdsServer& operator=(const UdsServer&) = delete;

  // Channel implementation
  void start() override;
  void stop() override;
  bool is_connected() const override;
  void async_write_copy(memory::ConstByteSpan data) override;
  void async_write_move(std::vector<uint8_t>&& data) override;
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override;
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Multi-client support
  bool broadcast(memory::ConstByteSpan message);
  bool send_to_client(size_t client_id, memory::ConstByteSpan message);
  size_t get_client_count() const;
  std::vector<size_t> get_connected_clients() const;

  using MultiClientConnectHandler = std::function<void(size_t client_id, const std::string& client_info)>;
  using MultiClientDataHandler = std::function<void(size_t client_id, const std::vector<uint8_t>& data)>;
  using MultiClientDisconnectHandler = std::function<void(size_t client_id)>;

  void on_multi_connect(MultiClientConnectHandler handler);
  void on_multi_data(MultiClientDataHandler handler);
  void on_multi_disconnect(MultiClientDisconnectHandler handler);

  base::LinkState get_state() const;

 private:
  explicit UdsServer(const config::UdsServerConfig& cfg);
  UdsServer(const config::UdsServerConfig& cfg, std::unique_ptr<interface::UdsAcceptorInterface> acceptor,
            boost::asio::io_context& ioc);

  struct Impl;
  const Impl* get_impl() const { return impl_.get(); }
  Impl* get_impl() { return impl_.get(); }
  std::unique_ptr<Impl> impl_;
};
}  // namespace transport
}  // namespace unilink
