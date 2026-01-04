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

#include <atomic>
#include <boost/asio.hpp>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/base/platform.hpp"
#include "unilink/common/thread_safe_state.hpp"
#include "unilink/base/visibility.hpp"
#include "unilink/config/serial_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/interface/iserial_port.hpp"

namespace unilink {
namespace transport {

using common::LinkState;
using common::ThreadSafeLinkState;
using config::SerialConfig;
using interface::Channel;
using interface::SerialPortInterface;
namespace net = boost::asio;

// Use static create() helpers to construct safely
class UNILINK_API Serial : public Channel, public std::enable_shared_from_this<Serial> {
 public:
  static std::shared_ptr<Serial> create(const SerialConfig& cfg);
  static std::shared_ptr<Serial> create(const SerialConfig& cfg, net::io_context& ioc);
  static std::shared_ptr<Serial> create(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port,
                                        net::io_context& ioc);
  ~Serial() override;

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t n) override;
  void async_write_move(std::vector<uint8_t>&& data) override;
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Dynamic configuration methods
  void set_retry_interval(unsigned interval_ms);

 private:
  explicit Serial(const SerialConfig& cfg);
  Serial(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port, net::io_context& ioc);
  void open_and_configure();
  void start_read();
  void do_write();
  void handle_error(const char* where, const boost::system::error_code& ec);
  void report_backpressure(size_t queued_bytes);
  void schedule_retry(const char* where, const boost::system::error_code&);
  void close_port();
  void notify_state();

 private:
  bool started_ = false;
  std::atomic<bool> stopping_{false};
  net::io_context& ioc_;
  bool owns_ioc_;
  net::strand<net::io_context::executor_type> strand_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::SerialPortInterface> port_;
  SerialConfig cfg_;
  net::steady_timer retry_timer_;

  std::vector<uint8_t> rx_;
  std::deque<std::variant<common::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>> tx_;
  bool writing_ = false;
  size_t queued_bytes_ = 0;
  size_t bp_high_;   // Configurable backpressure threshold
  size_t bp_limit_;  // Hard cap for queued bytes
  size_t bp_low_;    // Backpressure relief threshold
  bool backpressure_active_ = false;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  std::atomic<bool> opened_{false};
  ThreadSafeLinkState state_{LinkState::Idle};
};
}  // namespace transport
}  // namespace unilink
