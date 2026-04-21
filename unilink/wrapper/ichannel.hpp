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
#include "unilink/framer/iframer.hpp"
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
  [[nodiscard]] virtual std::future<bool> start() = 0;

  /**
   * @brief Synchronously start the channel/server and wait for the result.
   */
  [[nodiscard]] virtual bool start_sync() { return start().get(); }

  /**
   * @brief Stop the channel and block until all pending async operations are cancelled.
   *
   * Safe to call from any thread. After stop() returns, no further callbacks will fire
   * and it is safe to destroy the object. Calling stop() more than once is a no-op.
   */
  virtual void stop() = 0;
  virtual bool connected() const = 0;

  // Transmission
  /**
   * @brief Enqueue data for transmission.
   * @return true  Data was accepted into the send queue.
   * @return false Data was dropped — channel not connected or backpressure limit reached.
   *
   * The call is non-blocking. Delivery is not guaranteed even when true is returned;
   * network errors are reported via on_error().
   */
  virtual bool send(std::string_view data) = 0;

  /**
   * @brief Enqueue a line (data + "\n") for transmission.
   * @return true  Data was accepted into the send queue.
   * @return false Data was dropped — channel not connected or backpressure limit reached.
   */
  virtual bool send_line(std::string_view line) = 0;

  // Event handlers
  virtual ChannelInterface& on_data(MessageHandler handler) = 0;
  virtual ChannelInterface& on_connect(ConnectionHandler handler) = 0;
  virtual ChannelInterface& on_disconnect(ConnectionHandler handler) = 0;
  virtual ChannelInterface& on_error(ErrorHandler handler) = 0;

  /**
   * @brief Set a message framer for this channel.
   * @param framer The framer instance to use.
   */
  virtual ChannelInterface& framer(std::unique_ptr<framer::IFramer> framer) = 0;

  /**
   * @brief Set a handler for complete messages extracted by the framer.
   * @param handler The callback for framed messages.
   */
  virtual ChannelInterface& on_message(MessageHandler handler) = 0;

  // Management
  virtual ChannelInterface& auto_start(bool manage = true) = 0;
};

}  // namespace wrapper
}  // namespace unilink
