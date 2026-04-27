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
#include "unilink/framer/iframer.hpp"
#include "unilink/wrapper/context.hpp"

namespace unilink {
namespace wrapper {

/**
 * @brief Interface for 1:N server communication (e.g., TcpServer)
 */
class UNILINK_API ServerInterface {
 public:
  using MessageHandler = std::function<void(const MessageContext&)>;
  using BatchMessageHandler = std::function<void(const std::vector<MessageContext>&)>;
  using ConnectionHandler = std::function<void(const ConnectionContext&)>;
  using ErrorHandler = std::function<void(const ErrorContext&)>;
  using FramerFactory = std::function<std::unique_ptr<framer::IFramer>()>;

  virtual ~ServerInterface() = default;

  // Lifecycle
  [[nodiscard]] virtual std::future<bool> start() = 0;

  /**
   * @brief Synchronously start the channel/server and wait for the result.
   */
  [[nodiscard]] virtual bool start_sync() { return start().get(); }

  /**
   * @brief Stop the server and block until all active sessions are closed.
   *
   * Safe to call from any thread. After stop() returns, no further callbacks will fire
   * and it is safe to destroy the object. Calling stop() more than once is a no-op.
   */
  virtual void stop() = 0;
  virtual bool listening() const = 0;

  // Transmission
  //
  // Strategy-aware API (recommended):
  //
  //   send_to() / broadcast()
  //     Behaviour depends on the configured backpressure strategy:
  //       BestEffort — non-blocking; drops data when the target client's queue is full.
  //       Reliable   — blocks the calling thread until queue pressure is relieved,
  //                    then enqueues. For broadcast(), each client is waited on
  //                    sequentially.
  //
  // Explicit API (escape hatch):
  //
  //   try_send_to() / try_broadcast()
  //     Always non-blocking and always drops on full queue, regardless of strategy.
  //
  //   send_to_blocking()
  //     Always blocks until queue pressure is relieved, regardless of strategy.

  /**
   * @brief Send to a specific client, honouring the backpressure strategy.
   *
   * BestEffort: non-blocking, drops if the client's send queue is full.
   * Reliable:   blocks until queue pressure is relieved, then enqueues.
   *
   * @return true Data was accepted. @return false Dropped or client not found.
   */
  virtual bool send_to(ClientId client_id, std::string_view data) = 0;

  /**
   * @brief Send to all connected clients, honouring the backpressure strategy.
   * @return true At least one client accepted the data.
   */
  virtual bool broadcast(std::string_view data) = 0;

  /**
   * @brief Block until queue pressure is relieved, then send to a client. Ignores strategy.
   * @return true Data was accepted. @return false Server stopped while waiting.
   */
  virtual bool send_to_blocking(ClientId client_id, std::string_view data) = 0;

  /**
   * @brief Non-blocking send_to that always drops on a full queue, ignoring strategy.
   *
   * Use as an escape hatch when you need drop-on-full behaviour on a Reliable channel.
   *
   * @return true Data was accepted. @return false Dropped or client not found.
   */
  virtual bool try_send_to(ClientId client_id, std::string_view data) = 0;

  /**
   * @brief Non-blocking broadcast that always drops on full queues, ignoring strategy.
   * @return true At least one client accepted the data.
   */
  virtual bool try_broadcast(std::string_view data) = 0;

  // Event handlers
  virtual ServerInterface& on_connect(ConnectionHandler handler) = 0;
  virtual ServerInterface& on_disconnect(ConnectionHandler handler) = 0;
  virtual ServerInterface& on_data(MessageHandler handler) = 0;

  /** @brief Register a callback for batched data reception */
  virtual ServerInterface& on_data_batch(BatchMessageHandler handler) = 0;

  virtual ServerInterface& on_error(ErrorHandler handler) = 0;

  /**
   * @brief Register a callback to be notified when send queue congestion changes.
   * @param handler Callback receiving the current number of queued bytes.
   */
  virtual ServerInterface& on_backpressure(std::function<void(size_t)> handler) {
    (void)handler;
    return *this;
  }

  /**
   * @brief Set a factory function to create a new framer for each client connection.
   * @param factory Function that returns a unique_ptr to a new framer.
   */
  virtual ServerInterface& framer(FramerFactory factory) = 0;

  /**
   * @brief Set a handler for complete messages extracted by the framer.
   * @param handler callback taking MessageContext (where data is the framed payload).
   */
  virtual ServerInterface& on_message(MessageHandler handler) = 0;

  /** @brief Register a callback for batched framed message reception */
  virtual ServerInterface& on_message_batch(BatchMessageHandler handler) = 0;

  // Management
  virtual ServerInterface& auto_start(bool manage = true) = 0;
  virtual size_t client_count() const = 0;
  virtual std::vector<ClientId> connected_clients() const = 0;
};

}  // namespace wrapper
}  // namespace unilink
