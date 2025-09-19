#pragma once

#include <boost/asio.hpp>
#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "interface/ichannel.hpp"

namespace net = boost::asio;

struct SerialConfig {
  std::string device = "/dev/ttyUSB0";
  unsigned baud_rate = 115200;
  unsigned char_size = 8;  // 5,6,7,8
  enum class Parity { None, Even, Odd } parity = Parity::None;
  unsigned stop_bits = 1;  // 1 or 2
  enum class Flow { None, Software, Hardware } flow = Flow::None;

  size_t read_chunk = 4096;     // 1회 read 버퍼 크기
  bool reopen_on_error = true;  // 장치 분리/에러 시 재오픈 시도

  unsigned retry_interval_ms = 2000;  // 2s
};

class Serial : public IChannel, public std::enable_shared_from_this<Serial> {
 public:
  Serial(net::io_context& ioc, const SerialConfig& cfg);

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
  net::serial_port port_;
  SerialConfig cfg_;
  net::steady_timer retry_timer_;

  std::vector<uint8_t> rx_;
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queued_bytes_ = 0;
  const size_t bp_high_ = 1 << 20;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;

  bool opened_ = false;
  LinkState state_ = LinkState::Idle;
};
