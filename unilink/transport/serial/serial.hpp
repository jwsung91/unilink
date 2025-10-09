#pragma once

#include <boost/asio.hpp>
#include <cstddef>
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
#include "unilink/common/thread_safe_state.hpp"
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

class Serial : public Channel, public std::enable_shared_from_this<Serial> {
 public:
  explicit Serial(const SerialConfig& cfg);
  // Constructor for testing with dependency injection
  Serial(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port, net::io_context& ioc);
  ~Serial() override;

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t n) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // Dynamic configuration methods
  void set_retry_interval(unsigned interval_ms);

 private:
  void open_and_configure();
  void start_read();
  void do_write();
  void handle_error(const char* where, const boost::system::error_code& ec);
  void schedule_retry(const char* where, const boost::system::error_code&);
  void close_port();
  void notify_state();

 private:
  net::io_context& ioc_;
  bool owns_ioc_;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::SerialPortInterface> port_;
  SerialConfig cfg_;
  net::steady_timer retry_timer_;

  std::vector<uint8_t> rx_;
  std::deque<std::variant<common::PooledBuffer, std::vector<uint8_t>>> tx_;
  bool writing_ = false;
  size_t queued_bytes_ = 0;
  size_t bp_high_;  // Configurable backpressure threshold

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  bool opened_ = false;
  ThreadSafeLinkState state_{LinkState::Idle};
};
}  // namespace transport
}  // namespace unilink