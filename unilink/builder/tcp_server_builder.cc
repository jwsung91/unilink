#include "unilink/builder/tcp_server_builder.hpp"

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

TcpServerBuilder::TcpServerBuilder(uint16_t port)
    : port_(port),
      auto_start_(false),
      auto_manage_(false),
      use_independent_context_(false),
      enable_port_retry_(false),
      max_port_retries_(3),
      port_retry_interval_ms_(1000),
      max_clients_(SIZE_MAX),
      client_limit_set_(false) {}

std::unique_ptr<wrapper::TcpServer> TcpServerBuilder::build() {
  // Client limit validation
  if (!client_limit_set_) {
    throw std::runtime_error(
        "Client limit must be set before building server. Use single_client(), multi_client(n), or "
        "unlimited_clients()");
  }

  // IoContext 관리
  if (use_independent_context_) {
    // 독립적인 IoContext 사용 (테스트 격리용)
    // IoContextManager를 통해 독립적인 컨텍스트 생성
    auto independent_context = common::IoContextManager::instance().create_independent_context();
    // 현재는 기본 구현 유지, 향후 wrapper가 독립적인 컨텍스트를 받을 수 있도록 확장 가능
  } else {
    // 자동으로 IoContextManager 초기화 (기본 동작)
    AutoInitializer::ensure_io_context_running();
  }

  auto server = std::make_unique<wrapper::TcpServer>(port_);

  // Apply configuration
  if (auto_start_) {
    server->auto_start(true);
  }

  if (auto_manage_) {
    server->auto_manage(true);
  }

  // Set callbacks
  if (on_data_) {
    server->on_data(on_data_);
  }

  if (on_connect_) {
    server->on_connect(on_connect_);
  }

  if (on_disconnect_) {
    server->on_disconnect(on_disconnect_);
  }

  if (on_error_) {
    server->on_error(on_error_);
  }

  // 멀티 클라이언트 콜백 설정
  if (on_multi_connect_) {
    server->on_multi_connect(on_multi_connect_);
  }

  if (on_multi_data_) {
    server->on_multi_data(on_multi_data_);
  }

  if (on_multi_disconnect_) {
    server->on_multi_disconnect(on_multi_disconnect_);
  }

  // Port retry configuration
  std::cout << "DEBUG: enable_port_retry_ = " << enable_port_retry_ << std::endl;
  if (enable_port_retry_) {
    std::cout << "DEBUG: Setting port retry: max=" << max_port_retries_ << ", interval=" << port_retry_interval_ms_
              << std::endl;
    server->enable_port_retry(true, max_port_retries_, port_retry_interval_ms_);
  }

  return server;
}

TcpServerBuilder& TcpServerBuilder::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_data(std::function<void(const std::string&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_data(std::function<void(size_t, const std::string&)> handler) {
  on_multi_data_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_connect(std::function<void()> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_connect(std::function<void(size_t, const std::string&)> handler) {
  on_multi_connect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_disconnect(std::function<void()> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_disconnect(std::function<void(size_t)> handler) {
  on_multi_disconnect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_error(std::function<void(const std::string&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

// 멀티 클라이언트 지원 메서드 구현
TcpServerBuilder& TcpServerBuilder::on_multi_connect(std::function<void(size_t, const std::string&)> handler) {
  on_multi_connect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_multi_data(std::function<void(size_t, const std::string&)> handler) {
  on_multi_data_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::on_multi_disconnect(std::function<void(size_t)> handler) {
  on_multi_disconnect_ = std::move(handler);
  return *this;
}

TcpServerBuilder& TcpServerBuilder::enable_port_retry(bool enable, int max_retries, int retry_interval_ms) {
  enable_port_retry_ = enable;
  max_port_retries_ = max_retries;
  port_retry_interval_ms_ = retry_interval_ms;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::max_clients(size_t max) {
  if (max == 0) {
    throw std::invalid_argument("Use unlimited_clients() for unlimited connections");
  }
  if (max == 1) {
    throw std::invalid_argument("Use single_client() for single client mode");
  }
  max_clients_ = max;
  client_limit_set_ = true;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::single_client() {
  max_clients_ = 1;
  client_limit_set_ = true;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::multi_client(size_t max) {
  if (max == 0) {
    throw std::invalid_argument("Use unlimited_clients() for unlimited connections");
  }
  if (max == 1) {
    throw std::invalid_argument("Use single_client() for single client mode");
  }
  max_clients_ = max;
  client_limit_set_ = true;
  return *this;
}

TcpServerBuilder& TcpServerBuilder::unlimited_clients() {
  max_clients_ = 0;  // 0 = unlimited
  client_limit_set_ = true;
  return *this;
}

}  // namespace builder
}  // namespace unilink
