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
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <thread>
#include <variant>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/memory/memory_pool.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/base/visibility.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/interface/channel.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using udp = net::ip::udp;
using base::LinkState;
using concurrency::ThreadSafeLinkState;

class UNILINK_API UdpChannel : public interface::Channel, public std::enable_shared_from_this<UdpChannel> {
 public:
  static std::shared_ptr<UdpChannel> create(const config::UdpConfig& cfg);
  static std::shared_ptr<UdpChannel> create(const config::UdpConfig& cfg, net::io_context& ioc);
  ~UdpChannel();

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t size) override;
  void async_write_move(std::vector<uint8_t>&& data) override;
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override;

  // Callbacks must be configured before start() is invoked to avoid setter races.
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

 private:
  explicit UdpChannel(const config::UdpConfig& cfg);
  UdpChannel(const config::UdpConfig& cfg, net::io_context& ioc);

  void open_socket();
  void start_receive();
  void handle_receive(const boost::system::error_code& ec, std::size_t bytes);
  void do_write();
  void close_socket();
  void notify_state();
  void report_backpressure(size_t queued_bytes);
  size_t queued_bytes_front() const;
  bool enqueue_buffer(
      std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>&& buffer,
      size_t size);
  void set_remote_from_config();
  void transition_to(LinkState target, const boost::system::error_code& ec = {});
  void clear_callbacks();
  void perform_stop_cleanup();
  void reset_start_state();
  void join_ioc_thread(bool allow_detach);

 private:
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_;
  bool owns_ioc_{true};
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  udp::socket socket_;
  udp::endpoint local_endpoint_;
  udp::endpoint recv_endpoint_;
  std::optional<udp::endpoint> remote_endpoint_;

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>> tx_;
  bool writing_{false};
  size_t queue_bytes_{0};
  config::UdpConfig cfg_;
  size_t bp_high_;
  size_t bp_low_;
  size_t bp_limit_;
  bool backpressure_active_{false};

  std::atomic<bool> stop_requested_{false};
  std::atomic<bool> stopping_{false};
  std::atomic<bool> opened_{false};
  std::atomic<bool> connected_{false};
  bool started_{false};
  ThreadSafeLinkState state_{LinkState::Idle};
  std::atomic<bool> terminal_state_notified_{false};

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
};

}  // namespace transport
}  // namespace unilink
