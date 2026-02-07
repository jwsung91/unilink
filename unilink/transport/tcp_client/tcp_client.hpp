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
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/base/platform.hpp"
#include "unilink/base/visibility.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/memory/memory_pool.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using base::LinkState;
using concurrency::ThreadSafeLinkState;
using config::TcpClientConfig;
using interface::Channel;
using tcp = net::ip::tcp;

// Use static create() helpers to construct safely
class UNILINK_API TcpClient : public Channel, public std::enable_shared_from_this<TcpClient> {
 public:
  using BufferVariant =
      std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>;

  static std::shared_ptr<TcpClient> create(const TcpClientConfig& cfg);
  static std::shared_ptr<TcpClient> create(const TcpClientConfig& cfg, net::io_context& ioc);
  ~TcpClient();

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(memory::ConstByteSpan data) override;
  void async_write_move(std::vector<uint8_t>&& data) override;
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override;

  // Callbacks must be configured before start() is invoked to avoid setter races.
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Dynamic configuration methods
  void set_retry_interval(unsigned interval_ms);

 private:
  explicit TcpClient(const TcpClientConfig& cfg);
  explicit TcpClient(const TcpClientConfig& cfg, net::io_context& ioc);
  void do_resolve_connect();
  void schedule_retry();
  void start_read();
  void do_write();
  void handle_close(const boost::system::error_code& ec = {});
  void transition_to(LinkState next, const boost::system::error_code& ec = {});
  void perform_stop_cleanup();
  void reset_start_state();
  void join_ioc_thread(bool allow_detach);
  void close_socket();
  void recalculate_backpressure_bounds();
  void report_backpressure(size_t queued_bytes);
  void notify_state();
  void reset_io_objects();

 private:
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_ = nullptr;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;
  std::atomic<uint64_t> lifecycle_seq_{0};
  std::atomic<uint64_t> stop_seq_{0};
  std::atomic<uint64_t> current_seq_{0};
  tcp::resolver resolver_;
  tcp::socket socket_;
  TcpClientConfig cfg_;
  net::steady_timer retry_timer_;
  net::steady_timer connect_timer_;
  bool owns_ioc_ = true;
  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> stopping_{false};
  std::atomic<bool> terminal_state_notified_{false};

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<BufferVariant> tx_;
  std::optional<BufferVariant> current_write_buffer_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;   // Configurable backpressure threshold (high watermark)
  size_t bp_low_;    // Backpressure relief watermark
  size_t bp_limit_;  // Hard cap for queued bytes
  bool backpressure_active_ = false;
  unsigned first_retry_interval_ms_ = 100;  // Short first retry to reduce initial connection delay

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  std::atomic<bool> connected_{false};
  ThreadSafeLinkState state_{LinkState::Idle};
  int retry_attempts_ = 0;
};
}  // namespace transport
}  // namespace unilink
