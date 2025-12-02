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
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "unilink/common/constants.hpp"
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/common/platform.hpp"
#include "unilink/common/thread_safe_state.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/interface/channel.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpClientConfig;
using interface::Channel;
using tcp = net::ip::tcp;

class TcpClient : public Channel, public std::enable_shared_from_this<TcpClient> {
 public:
  explicit TcpClient(const TcpClientConfig& cfg);
  explicit TcpClient(const TcpClientConfig& cfg, net::io_context& ioc);
  ~TcpClient();

  void start() override;
  void stop(std::function<void()> on_stopped = nullptr) override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t size) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Dynamic configuration methods
  void set_retry_interval(unsigned interval_ms);

 private:
  void stop_internal(bool from_destructor, std::function<void()> on_stopped);
  void do_resolve_connect();
  void schedule_retry();
  void start_read();
  void do_write();
  void handle_close(const boost::system::error_code& ec = {});
  void close_socket();
  void notify_state();

 private:
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_ = nullptr;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;
  tcp::resolver resolver_;
  tcp::socket socket_;
  TcpClientConfig cfg_;
  net::steady_timer retry_timer_;
  bool owns_ioc_ = true;
  std::atomic<bool> stopping_{false};

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<std::variant<common::PooledBuffer, std::vector<uint8_t>>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;  // Configurable backpressure threshold

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  std::mutex callback_mutex_;
  std::atomic<bool> connected_{false};
  ThreadSafeLinkState state_{LinkState::Idle};
};
}  // namespace transport
}  // namespace unilink
