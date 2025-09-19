#pragma once

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include "interface/ichannel.hpp"
#include "tcp_server_session.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

struct TcpServerConfig {
  uint16_t port = 9000;
};

class TcpServer : public IChannel,
                  public std::enable_shared_from_this<TcpServer> {  // NOLINT
 public:
  TcpServer(net::io_context& ioc, const TcpServerConfig& cfg);

  void start() override;
  void stop() override;
  bool is_connected() const override;
  void async_write_copy(const uint8_t* data, size_t size) override;
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

 private:
  void do_accept();
  void notify_state();

 private:
  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  TcpServerConfig cfg_;
  std::shared_ptr<TcpServerSession> sess_;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  LinkState state_ = LinkState::Idle;
};
