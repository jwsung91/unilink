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

  // IoContext management
  if (use_independent_context_) {
    // Use independent IoContext (for test isolation)
    // Create independent context through IoContextManager
    auto independent_context = common::IoContextManager::instance().create_independent_context();
    // Currently maintaining default implementation, can be extended in future for wrapper to accept independent context
  } else {
    // Automatically initialize IoContextManager (default behavior)
    AutoInitializer::ensure_io_context_running();
  }

  auto server = std::make_unique<wrapper::TcpServer>(port_);

  // Apply client limit configuration
  std::cout << "DEBUG: Builder - client_limit_set_=" << client_limit_set_ << ", max_clients_=" << max_clients_
            << std::endl;
  if (client_limit_set_) {
    if (max_clients_ == 0) {
      // Unlimited clients
      std::cout << "DEBUG: Setting unlimited clients" << std::endl;
      server->set_unlimited_clients();
    } else {
      // Limited clients
      std::cout << "DEBUG: Setting client limit to " << max_clients_ << std::endl;
      server->set_client_limit(max_clients_);
    }
  } else {
    std::cout << "DEBUG: No client limit set!" << std::endl;
  }

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

  // Multi-client callback configuration
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

// Multi-client support method implementation
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
