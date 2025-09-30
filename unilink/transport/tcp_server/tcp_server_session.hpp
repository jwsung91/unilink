#pragma once

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include "unilink/interface/channel.hpp"
#include "unilink/interface/itcp_socket.hpp"
#include "unilink/common/constants.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using common::LinkState;
using interface::TcpSocketInterface;
using tcp = net::ip::tcp;

class TcpServerSession : public std::enable_shared_from_this<TcpServerSession> {
 public:
  using OnBytes = interface::Channel::OnBytes;
  using OnBackpressure = interface::Channel::OnBackpressure;
  using OnClose = std::function<void()>;

  TcpServerSession(net::io_context& ioc, tcp::socket sock, size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD);
  // Constructor for testing with dependency injection
  TcpServerSession(net::io_context& ioc, std::unique_ptr<interface::TcpSocketInterface> socket, size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD);

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
  std::unique_ptr<interface::TcpSocketInterface> socket_;
  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<std::vector<uint8_t>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;  // Configurable backpressure threshold

  OnBytes on_bytes_;
  OnBackpressure on_bp_;
  OnClose on_close_;
  bool alive_ = false;
};
}  // namespace transport
}  // namespace unilink