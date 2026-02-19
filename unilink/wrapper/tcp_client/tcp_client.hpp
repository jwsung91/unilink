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
 * @brief Modernized TCP Client Wrapper
 */
class UNILINK_API TcpClient : public ChannelInterface {
 public:
  TcpClient(const std::string& host, uint16_t port);
  TcpClient(const std::string& host, uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc);
  explicit TcpClient(std::shared_ptr<interface::Channel> channel);
  ~TcpClient() override;

  // ChannelInterface implementation
  std::future<bool> start() override;
  void stop() override;
  void send(std::string_view data) override;
  void send_line(std::string_view line) override;
  bool is_connected() const override;

  ChannelInterface& on_data(MessageHandler handler) override;
  ChannelInterface& on_connect(ConnectionHandler handler) override;
  ChannelInterface& on_disconnect(ConnectionHandler handler) override;
  ChannelInterface& on_error(ErrorHandler handler) override;

  ChannelInterface& auto_manage(bool manage = true) override;

  // Configuration
  TcpClient& set_retry_interval(std::chrono::milliseconds interval);
  TcpClient& set_max_retries(int max_retries);
  TcpClient& set_connection_timeout(std::chrono::milliseconds timeout);
  TcpClient& set_manage_external_context(bool manage);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace wrapper
}  // namespace unilink
