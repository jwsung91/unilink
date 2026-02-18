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

#include "unilink/base/visibility.hpp"
#include "unilink/wrapper/context.hpp"

namespace unilink {
namespace wrapper {

/**
 * @brief Common interface for 1:1 point-to-point communication (e.g., TcpClient, Serial, Udp)
 */
class UNILINK_API ChannelInterface {
 public:
  using MessageHandler = std::function<void(const MessageContext&)>;
  using ConnectionHandler = std::function<void(const ConnectionContext&)>;
  using ErrorHandler = std::function<void(const ErrorContext&)>;

  virtual ~ChannelInterface() = default;

  // Lifecycle
  virtual std::future<bool> start() = 0;
  virtual void stop() = 0;
  virtual bool is_connected() const = 0;

  // Transmission
  virtual void send(std::string_view data) = 0;
  virtual void send_line(std::string_view line) = 0;

  // Event handlers
  virtual ChannelInterface& on_data(MessageHandler handler) = 0;
  virtual ChannelInterface& on_connect(ConnectionHandler handler) = 0;
  virtual ChannelInterface& on_disconnect(ConnectionHandler handler) = 0;
  virtual ChannelInterface& on_error(ErrorHandler handler) = 0;

  // Management
  virtual ChannelInterface& auto_manage(bool manage = true) = 0;
};

}  // namespace wrapper
}  // namespace unilink
