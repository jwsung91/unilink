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
#include <vector>

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
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  bool started_{false};
  std::atomic<bool> is_listening_{false};

  bool auto_manage_{false};
  bool port_retry_enabled_{false};
  int max_port_retries_{3};
  int port_retry_interval_ms_{1000};
  int idle_timeout_ms_{0};
  bool client_limit_enabled_{false};
  size_t max_clients_{0};
  bool notify_send_failure_{false};

  ConnectionHandler on_client_connect_{nullptr};
  ConnectionHandler on_client_disconnect_{nullptr};
  MessageHandler on_data_{nullptr};
  ErrorHandler on_error_{nullptr};

  explicit Impl(uint16_t port) : port_(port), started_(false), is_listening_(false) {}

  Impl(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : port_(port),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr),
        manage_external_context_(false),
        started_(false),
        is_listening_(false) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel)
      : port_(0), channel_(std::move(channel)), started_(false), is_listening_(false) {
    setup_internal_handlers();
  }

  ~Impl() { try { stop(); } catch (...) {} }

  void fulfill_all(bool value) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : pending_promises_) { try { p.set_value(value); } catch (...) {} }
    pending_promises_.clear();
  }

  std::future<bool> start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_listening_) {
      std::promise<bool> p; p.set_value(true); return p.get_future();
    }
    std::promise<bool> p;
    auto f = p.get_future();
    pending_promises_.push_back(std::move(p));
    if (started_) return f;

    if (!channel_) {
      config::TcpServerConfig config;
      config.port = port_;
      config.enable_port_retry = port_retry_enabled_;
      config.max_port_retries = max_port_retries_;
      config.port_retry_interval_ms = port_retry_interval_ms_;
      config.idle_timeout_ms = idle_timeout_ms_;
      
      channel_ = factory::ChannelFactory::create(config, external_ioc_);
      setup_internal_handlers();
      
      auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
      if (transport_server && client_limit_enabled_) {
        if (max_clients_ == 0) transport_server->set_unlimited_clients();
        else transport_server->set_client_limit(max_clients_);
      }
    }
    channel_->start();
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(external_ioc_->get_executor());
      external_thread_ = std::thread([ioc = external_ioc_]() { try { ioc->run(); } catch(...) {} });
    }
    started_ = true;
    return f;
  }

  void stop() {
    bool should_join = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!started_) {
        for (auto& p : pending_promises_) { try { p.set_value(false); } catch(...) {} }
        pending_promises_.clear();
        return;
      }
      if (channel_) {
        channel_->on_bytes(nullptr);
        channel_->on_state(nullptr);
        auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
        if (transport_server) transport_server->request_stop();
        channel_->stop();
      }
      if (use_external_context_ && manage_external_context_) {
        if (work_guard_) work_guard_.reset();
        if (external_ioc_) external_ioc_->stop();
        should_join = true;
      }
      for (auto& p : pending_promises_) { try { p.set_value(false); } catch(...) {} }
      pending_promises_.clear();
      started_ = false;
      is_listening_ = false;
    }
    if (should_join && external_thread_.joinable()) {
      try { external_thread_.join(); } catch(...) {}
    }
    std::lock_guard<std::mutex> lock(mutex_);
    channel_.reset();
  }

  void setup_internal_handlers() {
    if (!channel_) return;
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
        fulfill_all(true);
      } else if (state == base::LinkState::Error || state == base::LinkState::Closed) {
        is_listening_ = false;
        fulfill_all(false);
        if (state == base::LinkState::Error && on_error_) {
          on_error_(ErrorContext(ErrorCode::IoError, "Server disconnected"));
        }
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
