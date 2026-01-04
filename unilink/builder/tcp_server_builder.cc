/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "unilink/builder/tcp_server_builder.hpp"

#include <boost/asio/io_context.hpp>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/diagnostics/exceptions.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/util/input_validator.hpp"

namespace unilink {
namespace builder {

TcpServerBuilder::TcpServerBuilder(uint16_t port)
    : port_(port),
      auto_manage_(false),
      use_independent_context_(false),
      enable_port_retry_(false),
      max_port_retries_(3),
      port_retry_interval_ms_(base::constants::DEFAULT_RETRY_INTERVAL_MS / 2),
      max_clients_(SIZE_MAX),
      client_limit_set_(false) {
  // Validate input parameters
  try {
    util::InputValidator::validate_port(port_);
  } catch (const diagnostics::ValidationException& e) {
    throw diagnostics::BuilderException("Invalid TCP server parameters: " + e.get_full_message(), "TcpServerBuilder",
                                        "constructor");
  }
}

std::unique_ptr<wrapper::TcpServer> TcpServerBuilder::build() {
  // Client limit validation
  if (!client_limit_set_) {
    throw std::runtime_error(
        "Client limit must be set before building server. Use single_client(), multi_client(n), or "
        "unlimited_clients()");
  }

  std::shared_ptr<boost::asio::io_context> external_ioc;
  // IoContext management
  if (use_independent_context_) {
    // Use independent IoContext (for test isolation)
    auto independent_context = concurrency::IoContextManager::instance().create_independent_context();
    external_ioc = std::shared_ptr<boost::asio::io_context>(std::move(independent_context));
  } else {
    // Automatically initialize IoContextManager (default behavior)
    AutoInitializer::ensure_io_context_running();
  }

  auto server = external_ioc ? std::make_unique<wrapper::TcpServer>(port_, external_ioc)
                             : std::make_unique<wrapper::TcpServer>(port_);

  if (external_ioc) {
    server->set_manage_external_context(use_independent_context_);
  }

  // Apply client limit configuration
  UNILINK_LOG_DEBUG(
      "tcp_server_builder", "build",
      "client_limit_set=" + std::to_string(client_limit_set_) + ", max_clients=" + std::to_string(max_clients_));
  if (client_limit_set_) {
    if (max_clients_ == 0) {
      // Unlimited clients
      UNILINK_LOG_DEBUG("tcp_server_builder", "build", "Setting unlimited clients");
      server->set_unlimited_clients();
    } else {
      // Limited clients
      UNILINK_LOG_DEBUG("tcp_server_builder", "build", "Setting client limit to " + std::to_string(max_clients_));
      server->set_client_limit(max_clients_);
    }
  } else {
    UNILINK_LOG_DEBUG("tcp_server_builder", "build", "No client limit set");
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
  UNILINK_LOG_DEBUG("tcp_server_builder", "build", "enable_port_retry=" + std::to_string(enable_port_retry_));
  if (enable_port_retry_) {
    UNILINK_LOG_DEBUG("tcp_server_builder", "build",
                      "Setting port retry: max=" + std::to_string(max_port_retries_) +
                          ", interval=" + std::to_string(port_retry_interval_ms_) + "ms");
    server->enable_port_retry(true, max_port_retries_, port_retry_interval_ms_);
  }

  // Apply configuration and auto-start if requested
  server->auto_manage(auto_manage_);

  return server;
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
