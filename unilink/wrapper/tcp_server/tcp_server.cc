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

#include "unilink/wrapper/tcp_server/tcp_server.hpp"

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {
namespace wrapper {

struct TcpServer::Impl {
  mutable std::mutex mutex_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;
  bool started_{false};
  bool auto_manage_{false};
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;

  // Start status notification
  std::promise<bool> start_promise_;
  bool start_promise_fulfilled_{false};

  // Configuration
  bool port_retry_enabled_{false};
  int max_port_retries_{3};
  int port_retry_interval_ms_{1000};
  int idle_timeout_ms_{0};
  bool client_limit_enabled_{false};
  size_t max_clients_{0};
  bool notify_send_failure_{false};

  // State
  std::atomic<bool> is_listening_{false};

  // Callbacks (Context based)
  ConnectionHandler on_client_connect_{nullptr};
  ConnectionHandler on_client_disconnect_{nullptr};
  MessageHandler on_data_{nullptr};
  ErrorHandler on_error_{nullptr};

  explicit Impl(uint16_t port) : port_(port) {}

  Impl(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : port_(port),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel) : port_(0), channel_(std::move(channel)) {
    setup_internal_handlers();
  }

  ~Impl() {
    try {
      stop();
    } catch (...) {}
  }

  std::future<bool> start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    // Reset promise for new start attempt
    start_promise_ = std::promise<bool>();
    start_promise_fulfilled_ = false;

    if (!channel_) {
      config::TcpServerConfig config;
      config.port = port_;
      config.enable_port_retry = port_retry_enabled_;
      config.max_port_retries = max_port_retries_;
      config.port_retry_interval_ms = port_retry_interval_ms_;
      config.idle_timeout_ms = idle_timeout_ms_;

      channel_ = factory::ChannelFactory::create(config, external_ioc_);
      setup_internal_handlers();

      if (client_limit_enabled_) {
        auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
        if (transport_server) {
          if (max_clients_ == 0) transport_server->set_unlimited_clients();
          else transport_server->set_client_limit(max_clients_);
        }
      }
    }

    channel_->start();
    
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      external_thread_ = std::thread([ioc = external_ioc_]() {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard(ioc->get_executor());
        ioc->run();
      });
    }
    
    started_ = true;
    return start_promise_.get_future();
  }

  void stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_) return;

    if (channel_) {
      channel_->on_bytes(nullptr);
      channel_->on_state(nullptr);
      
      auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
      if (transport_server) {
        transport_server->request_stop();
      }
      channel_->stop();
    }

    if (use_external_context_ && manage_external_context_ && external_thread_.joinable()) {
      if (external_ioc_) external_ioc_->stop();
      external_thread_.join();
    }

    is_listening_ = false;
    started_ = false;
    
    if (!start_promise_fulfilled_) {
      try { start_promise_.set_value(false); } catch(...) {}
      start_promise_fulfilled_ = true;
    }
  }

  void setup_internal_handlers() {
    if (!channel_) return;

    // Use transport specific handlers for multi-client support
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_connect([this](size_t id, const std::string& info) {
        if (on_client_connect_) on_client_connect_(ConnectionContext(id, info));
      });

      transport_server->on_multi_data([this](size_t id, const std::string& data) {
        if (on_data_) on_data_(MessageContext(id, data));
      });

      transport_server->on_multi_disconnect([this](size_t id) {
        if (on_client_disconnect_) on_client_disconnect_(ConnectionContext(id));
      });
    }

    channel_->on_state([this](base::LinkState state) {
      if (state == base::LinkState::Listening) {
        is_listening_ = true;
        if (!start_promise_fulfilled_) {
          start_promise_.set_value(true);
          start_promise_fulfilled_ = true;
        }
      } else if (state == base::LinkState::Error) {
        is_listening_ = false;
        if (!start_promise_fulfilled_) {
          start_promise_.set_value(false);
          start_promise_fulfilled_ = true;
        }
        if (on_error_) on_error_(ErrorContext(ErrorCode::StartFailed, "Server failed to start or encountered an error"));
      }
    });
  }
};

TcpServer::TcpServer(uint16_t port) : pimpl_(std::make_unique<Impl>(port)) {}
TcpServer::TcpServer(uint16_t port, std::shared_ptr<boost::asio::io_context> ioc) : pimpl_(std::make_unique<Impl>(port, ioc)) {}
TcpServer::TcpServer(std::shared_ptr<interface::Channel> ch) : pimpl_(std::make_unique<Impl>(ch)) {}
TcpServer::~TcpServer() = default;

std::future<bool> TcpServer::start() { return pimpl_->start(); }
void TcpServer::stop() { pimpl_->stop(); }
bool TcpServer::is_listening() const { return pimpl_->is_listening_.load(); }

bool TcpServer::broadcast(std::string_view data) {
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) return transport_server->broadcast(std::string(data));
  }
  return false;
}

bool TcpServer::send_to(size_t client_id, std::string_view data) {
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) return transport_server->send_to_client(client_id, std::string(data));
  }
  return false;
}

ServerInterface& TcpServer::on_client_connect(ConnectionHandler h) { pimpl_->on_client_connect_ = std::move(h); return *this; }
ServerInterface& TcpServer::on_client_disconnect(ConnectionHandler h) { pimpl_->on_client_disconnect_ = std::move(h); return *this; }
ServerInterface& TcpServer::on_data(MessageHandler h) { pimpl_->on_data_ = std::move(h); return *this; }
ServerInterface& TcpServer::on_error(ErrorHandler h) { pimpl_->on_error_ = std::move(h); return *this; }

size_t TcpServer::get_client_count() const {
  if (!pimpl_->channel_) return 0;
  auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
  return transport_server ? transport_server->get_client_count() : 0;
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  if (!pimpl_->channel_) return std::vector<size_t>();
  auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
  return transport_server ? transport_server->get_connected_clients() : std::vector<size_t>();
}

TcpServer& TcpServer::auto_manage(bool m) {
  pimpl_->auto_manage_ = m;
  if (pimpl_->auto_manage_ && !pimpl_->started_) start();
  return *this;
}

TcpServer& TcpServer::enable_port_retry(bool e, int m, int i) {
  pimpl_->port_retry_enabled_ = e; pimpl_->max_port_retries_ = m; pimpl_->port_retry_interval_ms_ = i;
  return *this;
}

TcpServer& TcpServer::idle_timeout(int ms) { pimpl_->idle_timeout_ms_ = ms; return *this; }
TcpServer& TcpServer::set_client_limit(size_t max) { pimpl_->max_clients_ = max; pimpl_->client_limit_enabled_ = true; return *this; }
TcpServer& TcpServer::set_unlimited_clients() { pimpl_->client_limit_enabled_ = false; return *this; }
TcpServer& TcpServer::notify_send_failure(bool e) { pimpl_->notify_send_failure_ = e; return *this; }
TcpServer& TcpServer::set_manage_external_context(bool m) { pimpl_->manage_external_context_ = m; return *this; }

}  // namespace wrapper
}  // namespace unilink
