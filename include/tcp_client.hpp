#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ichannel.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpClient : public IChannel, public std::enable_shared_from_this<TcpClient> {
 public:
  TcpClient(net::io_context& ioc, std::string host, uint16_t port);

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t size) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

 private:
  void do_resolve_connect();
  void schedule_retry();
  void start_read();
  void do_write();
  void handle_close();
  void close_socket();
  void notify_state();

 private:
  net::io_context& ioc_;
  tcp::resolver resolver_;
  tcp::socket socket_;
  std::string host_;
  uint16_t port_;
  net::steady_timer retry_timer_;
  unsigned retry_interval_ms_ = 2000;

  std::array<uint8_t, 4096> rx_{};
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  const size_t bp_high_ = 1 << 20;  // 1 MiB

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  bool connected_ = false;
  LinkState state_ = LinkState::Idle;
};


