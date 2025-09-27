#pragma once

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "unilink/config/tcp_server_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/interface/itcp_acceptor.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using namespace interface;
using namespace common;
using namespace config;
using tcp = net::ip::tcp;

class TcpServer : public Channel,
                  public std::enable_shared_from_this<TcpServer> {  // NOLINT
 public:
  explicit TcpServer(const TcpServerConfig& cfg);
  // Constructor for testing with dependency injection
  TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::ITcpAcceptor> acceptor,
            net::io_context& ioc);
  ~TcpServer();

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
  std::unique_ptr<net::io_context> owned_ioc_;
  bool owns_ioc_;
  net::io_context& ioc_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::ITcpAcceptor> acceptor_;
  TcpServerConfig cfg_;
  std::shared_ptr<TcpServerSession> sess_;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  LinkState state_ = LinkState::Idle;
};
}  // namespace transport
}  // namespace unilink