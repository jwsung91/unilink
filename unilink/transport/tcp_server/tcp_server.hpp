#pragma once

#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>

#include "unilink/config/tcp_server_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/interface/itcp_acceptor.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"
#include "unilink/common/thread_safe_state.hpp"
#include "unilink/common/error_handler.hpp"
#include "unilink/common/logger.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;

using interface::Channel;
using interface::TcpAcceptorInterface;
using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpServerConfig;
using tcp = net::ip::tcp;

class TcpServer : public Channel,
                  public std::enable_shared_from_this<TcpServer> {  // NOLINT
 public:
  explicit TcpServer(const TcpServerConfig& cfg);
  // Constructor for testing with dependency injection
  TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
            net::io_context& ioc);
  ~TcpServer();

  void start() override;
  void stop() override;
  bool is_connected() const override;
  void async_write_copy(const uint8_t* data, size_t size) override;
  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

  // 멀티 클라이언트 지원 메서드
  void broadcast(const std::string& message);
  void send_to_client(size_t client_id, const std::string& message);
  size_t get_client_count() const;
  std::vector<size_t> get_connected_clients() const;
  
  // 멀티 클라이언트 콜백 타입 정의
  using MultiClientConnectHandler = std::function<void(size_t client_id, const std::string& client_info)>;
  using MultiClientDataHandler = std::function<void(size_t client_id, const std::string& data)>;
  using MultiClientDisconnectHandler = std::function<void(size_t client_id)>;
  
  void on_multi_connect(MultiClientConnectHandler handler);
  void on_multi_data(MultiClientDataHandler handler);
  void on_multi_disconnect(MultiClientDisconnectHandler handler);

 private:
  void do_accept();
  void notify_state();

 private:
  std::unique_ptr<net::io_context> owned_ioc_;
  bool owns_ioc_;
  net::io_context& ioc_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::TcpAcceptorInterface> acceptor_;
  TcpServerConfig cfg_;
  
  // 멀티 클라이언트 지원
  std::vector<std::shared_ptr<TcpServerSession>> sessions_;
  mutable std::mutex sessions_mutex_;
  
  // 기존 API 호환성을 위한 현재 활성 세션
  std::shared_ptr<TcpServerSession> current_session_;

  // 멀티 클라이언트 콜백들
  MultiClientConnectHandler on_multi_connect_;
  MultiClientDataHandler on_multi_data_;
  MultiClientDisconnectHandler on_multi_disconnect_;

  // 기존 멤버들 (호환성 유지)
  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  ThreadSafeLinkState state_{LinkState::Idle};
};
}  // namespace transport
}  // namespace unilink