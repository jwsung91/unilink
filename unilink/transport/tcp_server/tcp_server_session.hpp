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

#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/base/platform.hpp"
#include "unilink/base/visibility.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/interface/itcp_socket.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using common::LinkState;
using interface::TcpSocketInterface;
using tcp = net::ip::tcp;

class UNILINK_API TcpServerSession : public std::enable_shared_from_this<TcpServerSession> {
 public:
  using OnBytes = interface::Channel::OnBytes;
  using OnBackpressure = interface::Channel::OnBackpressure;
  using OnClose = std::function<void()>;

  TcpServerSession(net::io_context& ioc, tcp::socket sock,
                   size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD);
  // Constructor for testing with dependency injection
  TcpServerSession(net::io_context& ioc, std::unique_ptr<interface::TcpSocketInterface> socket,
                   size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD);

  void start();
  void async_write_copy(const uint8_t* data, size_t size);
  void async_write_move(std::vector<uint8_t>&& data);
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data);
  void on_bytes(OnBytes cb);
  void on_backpressure(OnBackpressure cb);
  void on_close(OnClose cb);
  bool alive() const;
  void stop();

 private:
  void start_read();
  void do_write();
  void do_close();
  void report_backpressure(size_t queued_bytes);

 private:
  net::io_context& ioc_;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<interface::TcpSocketInterface> socket_;
  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<std::variant<common::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;   // Configurable backpressure threshold
  size_t bp_limit_;  // Hard cap for queued bytes
  size_t bp_low_;    // Backpressure relief threshold
  bool backpressure_active_ = false;

  OnBytes on_bytes_;
  OnBackpressure on_bp_;
  OnClose on_close_;
  std::atomic<bool> alive_{false};
};
}  // namespace transport
}  // namespace unilink
