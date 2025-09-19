#pragma once

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include "ichannel.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpServerSession : public std::enable_shared_from_this<TcpServerSession> {
 public:
  using OnBytes = IChannel::OnBytes;
  using OnBackpressure = IChannel::OnBackpressure;
  using OnClose = std::function<void()>;

  TcpServerSession(net::io_context& ioc, tcp::socket sock);

  void start();
  void async_write_copy(const uint8_t* data, size_t size);
  void on_bytes(OnBytes cb);
  void on_backpressure(OnBackpressure cb);
  void on_close(OnClose cb);
  bool alive() const;

 private:
  void start_read();
  void do_write();
  void do_close();

 private:
  net::io_context& ioc_;
  tcp::socket socket_;
  std::array<uint8_t, 4096> rx_{};
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  const size_t bp_high_ = 1 << 20;  // 1 MiB

  OnBytes on_bytes_;
  OnBackpressure on_bp_;
  OnClose on_close_;
  bool alive_ = false;
};
