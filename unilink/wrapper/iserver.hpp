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
#include <string_view>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/wrapper/context.hpp"

namespace unilink {
namespace wrapper {

/**
 * @brief Interface for 1:N server communication (e.g., TcpServer)
 */
class UNILINK_API ServerInterface {
 public:
  using MessageHandler = std::function<void(const MessageContext&)>;
  using ConnectionHandler = std::function<void(const ConnectionContext&)>;
  using ErrorHandler = std::function<void(const ErrorContext&)>;

  virtual ~ServerInterface() = default;

  // Lifecycle
  virtual std::future<bool> start() = 0;
  virtual void stop() = 0;
  virtual bool is_listening() const = 0;

  // Transmission
  virtual void broadcast(std::string_view data) = 0;
  virtual bool send_to(size_t client_id, std::string_view data) = 0;

  // Event handlers
  virtual ServerInterface& on_client_connect(ConnectionHandler handler) = 0;
  virtual ServerInterface& on_client_disconnect(ConnectionHandler handler) = 0;
  virtual ServerInterface& on_data(MessageHandler handler) = 0;
  virtual ServerInterface& on_error(ErrorHandler handler) = 0;

  // Management
  virtual size_t get_client_count() const = 0;
  virtual std::vector<size_t> get_connected_clients() const = 0;
};

}  // namespace wrapper
}  // namespace unilink
