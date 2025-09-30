#pragma once

#include <boost/asio.hpp>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "unilink/config/serial_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/interface/iserial_port.hpp"
#include "unilink/common/constants.hpp"

namespace unilink {
namespace transport {

using interface::Channel;
using interface::SerialPortInterface;
using common::LinkState;
using config::SerialConfig;
namespace net = boost::asio;

class Serial : public Channel, public std::enable_shared_from_this<Serial> {
 public:
  explicit Serial(const SerialConfig& cfg);
  // Constructor for testing with dependency injection
  Serial(const SerialConfig& cfg, std::unique_ptr<interface::SerialPortInterface> port,
         net::io_context& ioc);
  ~Serial() override;

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t n) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

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
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>>
      work_guard_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::SerialPortInterface> port_;
  SerialConfig cfg_;
  net::steady_timer retry_timer_;

  std::vector<uint8_t> rx_;
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queued_bytes_ = 0;
  const size_t bp_high_ = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  bool opened_ = false;
  LinkState state_ = LinkState::Idle;
};
}  // namespace transport
}  // namespace unilink