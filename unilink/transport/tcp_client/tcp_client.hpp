#pragma once

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <variant>

#include "unilink/config/tcp_client_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/common/constants.hpp"
#include "unilink/common/memory_pool.hpp"
#include "unilink/common/thread_safe_state.hpp"
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using interface::Channel;
using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpClientConfig;
using tcp = net::ip::tcp;

class TcpClient : public Channel,
                  public std::enable_shared_from_this<TcpClient> {
 public:
  explicit TcpClient(const TcpClientConfig& cfg);
  explicit TcpClient(const TcpClientConfig& cfg, net::io_context& ioc);
  ~TcpClient();

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(const uint8_t* data, size_t size) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;
  
  // Dynamic configuration methods
  void set_retry_interval(unsigned interval_ms);

 private:
  void do_resolve_connect();
  void schedule_retry();
  void start_read();
  void do_write();
  void handle_close();
  void close_socket();
  void notify_state();

 private:
  std::unique_ptr<net::io_context> ioc_;
  std::thread ioc_thread_;
  bool owns_ioc_ = true;

  tcp::resolver resolver_;
  tcp::socket socket_;
  TcpClientConfig cfg_;
  net::steady_timer retry_timer_;

  std::array<uint8_t, common::constants::DEFAULT_READ_BUFFER_SIZE> rx_{};
  std::deque<std::variant<common::PooledBuffer, std::vector<uint8_t>>> tx_;
  bool writing_ = false;
  size_t queue_bytes_ = 0;
  size_t bp_high_;  // Configurable backpressure threshold

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  bool connected_ = false;
  ThreadSafeLinkState state_{LinkState::Idle};
};
}  // namespace transport
}  // namespace unilink