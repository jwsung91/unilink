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
#include <vector>

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
  using BatchMessageHandler = std::function<void(const std::vector<MessageContext>&)>;
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
  //
  // Strategy-aware API (recommended):
  //
  //   send() / send_line()
  //     Behaviour depends on the configured backpressure strategy:
  //       BestEffort — non-blocking; drops data when the send queue is full.
  //       Reliable   — blocks the calling thread until queue pressure is relieved,
  //                    then enqueues. Never drops due to backpressure alone.
  //
  // Explicit API (escape hatch):
  //
  //   try_send() / try_send_line()
  //     Always non-blocking and always drops on full queue, regardless of strategy.
  //     Use when you need BestEffort behaviour on a Reliable channel for a specific
  //     call (e.g. fire-and-forget heartbeat).
  //
  //   send_blocking() / send_line_blocking()
  //     Always blocks until queue pressure is relieved, regardless of strategy.

  /**
   * @brief Enqueue data for transmission, honouring the backpressure strategy.
   *
   * BestEffort: non-blocking, drops if the send queue is full.
   * Reliable:   blocks until queue pressure is relieved, then enqueues.
   *
   * @return true  Data was accepted into the send queue.
   * @return false Data was dropped (not connected, or BestEffort queue full).
   */
  virtual bool send(std::string_view data) = 0;

  /**
   * @brief Enqueue a line (data + "\n") for transmission, honouring the backpressure strategy.
   * @return true  Data was accepted. @return false Data was dropped.
   */
  virtual bool send_line(std::string_view line) = 0;

  /**
   * @brief Block the calling thread until queue pressure is relieved, then enqueue.
   *
   * Ignores the configured strategy — always blocks. Prefer send() unless you need
   * to override BestEffort behaviour for a specific call.
   *
   * @return true Data was accepted. @return false Channel stopped while waiting.
   */
  virtual bool send_blocking(std::string_view data) = 0;

  /**
   * @brief Blocking variant of send_line(). Always blocks regardless of strategy.
   * @return true Data was accepted. @return false Channel stopped while waiting.
   */
  virtual bool send_line_blocking(std::string_view line) = 0;

  /**
   * @brief Non-blocking send that always drops on a full queue, ignoring strategy.
   *
   * Use as an escape hatch when you need drop-on-full behaviour on a Reliable channel.
   *
   * @return true Data was accepted. @return false Dropped (not connected or queue full).
   */
  virtual bool try_send(std::string_view data) = 0;

  /**
   * @brief Non-blocking send_line that always drops on a full queue, ignoring strategy.
   * @return true Data was accepted. @return false Dropped.
   */
  virtual bool try_send_line(std::string_view line) = 0;

  // Event handlers
  virtual ChannelInterface& on_data(MessageHandler handler) = 0;

  /** @brief Register a callback for batched data reception */
  virtual ChannelInterface& on_data_batch(BatchMessageHandler handler) = 0;

  virtual ChannelInterface& on_connect(ConnectionHandler handler) = 0;
  virtual ChannelInterface& on_disconnect(ConnectionHandler handler) = 0;
  virtual ChannelInterface& on_error(ErrorHandler handler) = 0;

  /**
   * @brief Register a callback to be notified when send queue congestion changes.
   * @param handler Callback receiving the current number of queued bytes.
   */
  virtual ChannelInterface& on_backpressure(std::function<void(size_t)> handler) {
    (void)handler;
    return *this;
  }

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

  /** @brief Register a callback for batched framed message reception */
  virtual ChannelInterface& on_message_batch(BatchMessageHandler handler) = 0;

  // Management
  virtual ChannelInterface& auto_start(bool manage = true) = 0;
};

}  // namespace wrapper
}  // namespace unilink
