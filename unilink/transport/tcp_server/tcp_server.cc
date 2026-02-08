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

#include "unilink/transport/tcp_server/tcp_server.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/error_handler.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/interface/itcp_acceptor.hpp"
#include "unilink/transport/tcp_server/boost_tcp_acceptor.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

using base::LinkState;
using concurrency::ThreadSafeLinkState;
using config::TcpServerConfig;
using interface::Channel;
using interface::TcpAcceptorInterface;

struct TcpServer::Impl {
  std::unique_ptr<net::io_context> owned_ioc_;
  bool owns_ioc_;
  bool uses_global_ioc_;
  net::io_context& ioc_;
  std::thread ioc_thread_;

  std::unique_ptr<interface::TcpAcceptorInterface> acceptor_;
  TcpServerConfig cfg_;

  std::unordered_map<size_t, std::shared_ptr<TcpServerSession>> sessions_;
  mutable std::shared_mutex sessions_mutex_;

  size_t max_clients_;
  bool client_limit_enabled_;
  bool paused_accept_ = false;

  std::shared_ptr<TcpServerSession> current_session_;

  MultiClientConnectHandler on_multi_connect_;
  MultiClientDataHandler on_multi_data_;
  MultiClientDisconnectHandler on_multi_disconnect_;

  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  ThreadSafeLinkState state_{LinkState::Idle};

  std::atomic<bool> stopping_{false};
  std::atomic<size_t> next_client_id_{0};

  Impl(const TcpServerConfig& cfg, net::io_context* ioc_ptr)
      : owned_ioc_(nullptr),
        owns_ioc_(false),
        uses_global_ioc_(!ioc_ptr),
        ioc_(ioc_ptr ? *ioc_ptr : concurrency::IoContextManager::instance().get_context()),
        acceptor_(nullptr),
        cfg_(cfg),
        max_clients_(cfg.max_connections > 0 ? static_cast<size_t>(cfg.max_connections) : 0),
        client_limit_enabled_(cfg.max_connections > 0),
        current_session_(nullptr) {
    // Create acceptor after all members are initialized
    if (!ioc_ptr) {
      // Using global IOC or null
    }
  }

  // Helper to initialize acceptor
  void init_acceptor(std::unique_ptr<interface::TcpAcceptorInterface> acceptor = nullptr) {
    if (acceptor) {
      acceptor_ = std::move(acceptor);
    } else {
      try {
        acceptor_ = std::make_unique<BoostTcpAcceptor>(ioc_);
      } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create TCP acceptor: " + std::string(e.what()));
      }
    }
  }

  void start(std::shared_ptr<TcpServer> self);
  void stop(std::shared_ptr<TcpServer> self);
  void request_stop(std::shared_ptr<TcpServer> self);
  void do_accept(std::shared_ptr<TcpServer> self);
  void notify_state();
  void attempt_port_binding(std::shared_ptr<TcpServer> self, int retry_count);
};

// ============================================================================
// TcpServer Implementation
// ============================================================================

std::shared_ptr<TcpServer> TcpServer::create(const TcpServerConfig& cfg) {
  return std::shared_ptr<TcpServer>(new TcpServer(cfg));
}

std::shared_ptr<TcpServer> TcpServer::create(const TcpServerConfig& cfg,
                                             std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                                             boost::asio::io_context& ioc) {
  return std::shared_ptr<TcpServer>(new TcpServer(cfg, std::move(acceptor), ioc));
}

TcpServer::TcpServer(const TcpServerConfig& cfg) : impl_(std::make_unique<Impl>(cfg, nullptr)) {
  impl_->init_acceptor();
}

TcpServer::TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                     boost::asio::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, &ioc)) {
  impl_->init_acceptor(std::move(acceptor));
  // Ensure acceptor is properly initialized
  if (!impl_->acceptor_) {
    throw std::runtime_error("Failed to create TCP acceptor");
  }
}

TcpServer::~TcpServer() {
  if (!impl_->state_.is_state(base::LinkState::Closed)) {
    stop();
  }
}

void TcpServer::start() { impl_->start(shared_from_this()); }

void TcpServer::stop() {
  std::shared_ptr<TcpServer> self;
  try {
    self = shared_from_this();
  } catch (const std::bad_weak_ptr&) {
    // Destructor or not managed by shared_ptr
  }
  impl_->stop(self);
}

bool TcpServer::is_connected() const {
  std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  return impl_->current_session_ && impl_->current_session_->alive();
}

void TcpServer::async_write_copy(memory::ConstByteSpan data) {
  if (impl_->stopping_.load()) return;
  std::shared_ptr<TcpServerSession> session;
  {
    std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
    session = impl_->current_session_;
  }

  if (session && session->alive()) {
    session->async_write_copy(data);
  }
}

void TcpServer::async_write_move(std::vector<uint8_t>&& data) {
  if (impl_->stopping_.load()) return;
  std::shared_ptr<TcpServerSession> session;
  {
    std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
    session = impl_->current_session_;
  }

  if (session && session->alive()) {
    session->async_write_move(std::move(data));
  }
}

void TcpServer::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (impl_->stopping_.load() || !data) return;
  std::shared_ptr<TcpServerSession> session;
  {
    std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
    session = impl_->current_session_;
  }

  if (session && session->alive()) {
    session->async_write_shared(std::move(data));
  }
}

void TcpServer::on_bytes(OnBytes cb) {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->on_bytes_ = std::move(cb);
}

void TcpServer::on_state(OnState cb) {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->on_state_ = std::move(cb);
}

void TcpServer::on_backpressure(OnBackpressure cb) {
  {
    std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
    impl_->on_bp_ = std::move(cb);
  }
  std::shared_ptr<TcpServerSession> session;
  {
    std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
    session = impl_->current_session_;
  }

  if (session) session->on_backpressure(impl_->on_bp_);
}

bool TcpServer::broadcast(std::string_view message) {
  // Safe copy: create shared_ptr<vector> *before* async calls.
  // This ensures the data persists even if the string_view source dies.
  auto shared_data = std::make_shared<const std::vector<uint8_t>>(message.begin(), message.end());

  std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  bool sent = false;
  for (auto& entry : impl_->sessions_) {
    auto& session = entry.second;
    if (session && session->alive()) {
      session->async_write_shared(shared_data);
      sent = true;
    }
  }

  return sent;
}

bool TcpServer::send_to_client(size_t client_id, std::string_view message) {
  // Safe copy: Convert string_view to a vector *synchronously* here.
  // We can use async_write_move to transfer ownership.
  std::vector<uint8_t> data(message.begin(), message.end());

  std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);

  auto it = impl_->sessions_.find(client_id);
  if (it != impl_->sessions_.end() && it->second && it->second->alive()) {
    // async_write_move takes rvalue ref and will move it into its internal queue
    it->second->async_write_move(std::move(data));
    return true;
  }
  UNILINK_LOG_DEBUG("tcp_server", "send_to_client",
                    "Send failed: client_id " + std::to_string(client_id) + " not found");
  return false;
}

size_t TcpServer::get_client_count() const {
  std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  size_t alive = 0;
  for (const auto& entry : impl_->sessions_) {
    if (entry.second && entry.second->alive()) {
      ++alive;
    }
  }
  return alive;
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  std::shared_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  std::vector<size_t> connected_clients;

  for (const auto& entry : impl_->sessions_) {
    if (entry.second && entry.second->alive()) {
      connected_clients.push_back(entry.first);
    }
  }

  return connected_clients;
}

void TcpServer::request_stop() { impl_->request_stop(shared_from_this()); }

void TcpServer::on_multi_connect(MultiClientConnectHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->on_multi_connect_ = std::move(handler);
}

void TcpServer::on_multi_data(MultiClientDataHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->on_multi_data_ = std::move(handler);
}

void TcpServer::on_multi_disconnect(MultiClientDisconnectHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->on_multi_disconnect_ = std::move(handler);
}

void TcpServer::set_client_limit(size_t max_clients) {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->max_clients_ = max_clients;
  impl_->client_limit_enabled_ = true;

  if (impl_->paused_accept_ && impl_->sessions_.size() < impl_->max_clients_) {
    impl_->paused_accept_ = false;
    net::post(impl_->ioc_, [self = shared_from_this()] { self->impl_->do_accept(self); });
  }
}

void TcpServer::set_unlimited_clients() {
  std::unique_lock<std::shared_mutex> lock(impl_->sessions_mutex_);
  impl_->client_limit_enabled_ = false;
  impl_->max_clients_ = 0;

  if (impl_->paused_accept_) {
    impl_->paused_accept_ = false;
    net::post(impl_->ioc_, [self = shared_from_this()] { self->impl_->do_accept(self); });
  }
}

base::LinkState TcpServer::get_state() const { return impl_->state_.get_state(); }

// ============================================================================
// TcpServer::Impl Implementation
// ============================================================================

void TcpServer::Impl::start(std::shared_ptr<TcpServer> self) {
  auto current = state_.get_state();
  if (current == base::LinkState::Listening || current == base::LinkState::Connected ||
      current == base::LinkState::Connecting) {
    UNILINK_LOG_DEBUG("tcp_server", "start", "Start called while already active, ignoring");
    return;
  }
  stopping_.store(false);

  if (uses_global_ioc_) {
    auto& manager = concurrency::IoContextManager::instance();
    if (!manager.is_running()) {
      manager.start();
    }
  }

  if (!acceptor_) {
    UNILINK_LOG_ERROR("tcp_server", "start", "Acceptor is null");
    diagnostics::error_reporting::report_system_error("tcp_server", "start", "Acceptor is null");
    state_.set_state(base::LinkState::Error);
    notify_state();
    return;
  }

  if (owns_ioc_) {
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }

  if (ioc_.get_executor().running_in_this_thread()) {
    if (!stopping_.load()) {
      attempt_port_binding(self, 0);
    }
  } else {
    net::dispatch(ioc_, [self] {
      if (self->impl_->stopping_.load()) return;
      self->impl_->attempt_port_binding(self, 0);
    });
  }
}

void TcpServer::Impl::stop(std::shared_ptr<TcpServer> self) {
  if (stopping_.exchange(true)) {
    return;
  }

  {
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    on_bytes_ = nullptr;
    on_state_ = nullptr;
    on_bp_ = nullptr;
    on_multi_connect_ = nullptr;
    on_multi_data_ = nullptr;
    on_multi_disconnect_ = nullptr;
  }

  auto cleanup = [this, self]() {
    boost::system::error_code ec;
    if (acceptor_ && acceptor_->is_open()) {
      acceptor_->close(ec);
    }

    std::vector<std::shared_ptr<TcpServerSession>> sessions_copy;
    {
      std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
      sessions_copy.reserve(sessions_.size());
      for (auto& kv : sessions_) {
        sessions_copy.push_back(kv.second);
      }
      sessions_.clear();
      current_session_.reset();
    }

    for (auto& session : sessions_copy) {
      if (session) {
        session->stop();
      }
    }

    state_.set_state(base::LinkState::Closed);
  };

  const bool in_ioc_thread = ioc_.get_executor().running_in_this_thread();

  if (in_ioc_thread) {
    cleanup();
    if (owns_ioc_) {
      ioc_.stop();
    }
    return;
  }

  bool dispatched = false;
  const bool has_active_ioc_thread =
      owns_ioc_ || (uses_global_ioc_ && concurrency::IoContextManager::instance().is_running());

  if (has_active_ioc_thread && self) {
    try {
      auto cleanup_promise = std::make_shared<std::promise<void>>();
      auto cleanup_future = cleanup_promise->get_future();

      net::dispatch(ioc_, [self, cleanup, cleanup_promise] {
        cleanup();
        cleanup_promise->set_value();
      });

      cleanup_future.wait();
      dispatched = true;
    } catch (const std::bad_weak_ptr&) {
      UNILINK_LOG_DEBUG("tcp_server", "stop", "shared_from_this() failed, performing synchronous cleanup");
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server", "stop", "Dispatch failed: " + std::string(e.what()));
    }
  }

  if (!dispatched) {
    cleanup();
  }

  if (owns_ioc_ && ioc_thread_.joinable()) {
    ioc_.stop();
    ioc_thread_.join();
    ioc_.restart();
  }
}

void TcpServer::Impl::request_stop(std::shared_ptr<TcpServer> self) {
  if (stopping_.load()) return;
  net::post(ioc_, [self] { self->stop(); });
}

void TcpServer::Impl::attempt_port_binding(std::shared_ptr<TcpServer> self, int retry_count) {
  if (stopping_.load()) return;
  boost::system::error_code ec;

  if (retry_count > 0) {
    UNILINK_LOG_DEBUG("tcp_server", "bind",
                      "Attempting port binding - retry enabled: " + std::to_string(cfg_.enable_port_retry) +
                          ", max retries: " + std::to_string(cfg_.max_port_retries) +
                          ", retry count: " + std::to_string(retry_count));
  }

  if (!acceptor_->is_open()) {
    acceptor_->open(tcp::v4(), ec);
    if (ec) {
      UNILINK_LOG_ERROR("tcp_server", "open", "Failed to open acceptor: " + ec.message());
      diagnostics::error_reporting::report_connection_error("tcp_server", "open", ec, false);
      state_.set_state(base::LinkState::Error);
      notify_state();
      return;
    }
  }

  acceptor_->bind(tcp::endpoint(tcp::v4(), cfg_.port), ec);
  if (ec) {
    if (cfg_.enable_port_retry && retry_count < cfg_.max_port_retries) {
      UNILINK_LOG_WARNING("tcp_server", "bind",
                          "Failed to bind to port " + std::to_string(cfg_.port) + " (attempt " +
                              std::to_string(retry_count + 1) + "/" + std::to_string(cfg_.max_port_retries) + "): " +
                              ec.message() + ". Retrying in " + std::to_string(cfg_.port_retry_interval_ms) + "ms...");

      auto timer = std::make_shared<net::steady_timer>(ioc_);
      timer->expires_after(std::chrono::milliseconds(cfg_.port_retry_interval_ms));
      timer->async_wait([self, retry_count, timer](const boost::system::error_code& timer_ec) {
        if (!timer_ec) {
          self->impl_->attempt_port_binding(self, retry_count + 1);
        }
      });
      return;
    } else {
      std::string error_msg = "Failed to bind to port: " + std::to_string(cfg_.port) + " - " + ec.message();
      if (cfg_.enable_port_retry) {
        error_msg += " (after " + std::to_string(retry_count) + " retries)";
      }
      UNILINK_LOG_ERROR("tcp_server", "bind", error_msg);
      diagnostics::error_reporting::report_connection_error("tcp_server", "bind", ec, false);
      state_.set_state(base::LinkState::Error);
      notify_state();
      return;
    }
  }

  acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    UNILINK_LOG_ERROR("tcp_server", "listen",
                      "Failed to listen on port: " + std::to_string(cfg_.port) + " - " + ec.message());
    diagnostics::error_reporting::report_connection_error("tcp_server", "listen", ec, false);
    state_.set_state(base::LinkState::Error);
    notify_state();
    return;
  }

  if (retry_count > 0) {
    UNILINK_LOG_INFO("tcp_server", "bind",
                     "Successfully bound to port " + std::to_string(cfg_.port) + " after " +
                         std::to_string(retry_count) + " retries");
  } else {
    UNILINK_LOG_INFO("tcp_server", "bind", "Successfully bound to port " + std::to_string(cfg_.port));
  }

  state_.set_state(base::LinkState::Listening);
  notify_state();
  do_accept(self);
}

void TcpServer::Impl::do_accept(std::shared_ptr<TcpServer> self) {
  if (stopping_.load() || !acceptor_ || !acceptor_->is_open()) return;

  acceptor_->async_accept([self](auto ec, tcp::socket sock) {
    if (self->impl_->stopping_.load()) {
      return;
    }
    if (ec) {
      if (ec == boost::asio::error::operation_aborted) {
        UNILINK_LOG_DEBUG("tcp_server", "accept", "Accept canceled (server shutting down)");
      } else {
        UNILINK_LOG_ERROR("tcp_server", "accept", "Accept error: " + ec.message());
        diagnostics::error_reporting::report_connection_error("tcp_server", "accept", ec, true);
        self->impl_->state_.set_state(base::LinkState::Error);
        self->impl_->notify_state();
      }
      if (!self->impl_->state_.is_state(base::LinkState::Closed) && !self->impl_->stopping_.load()) {
        auto timer = std::make_shared<net::steady_timer>(self->impl_->ioc_);
        timer->expires_after(std::chrono::milliseconds(100));
        timer->async_wait([self, timer](const boost::system::error_code&) {
          if (!self->impl_->stopping_.load()) {
            self->impl_->do_accept(self);
          }
        });
      }
      return;
    }

    boost::system::error_code ep_ec;
    auto rep = sock.remote_endpoint(ep_ec);
    std::string client_info = "unknown";
    if (!ep_ec) {
      client_info = rep.address().to_string() + ":" + std::to_string(rep.port());
    }

    if (self->impl_->client_limit_enabled_) {
      std::unique_lock<std::shared_mutex> lock(self->impl_->sessions_mutex_);
      if (self->impl_->sessions_.size() >= self->impl_->max_clients_) {
        UNILINK_LOG_WARNING("tcp_server", "accept",
                            "Client connection rejected - server at capacity (" +
                                std::to_string(self->impl_->sessions_.size()) + "/" +
                                std::to_string(self->impl_->max_clients_) + "): " + client_info);

        boost::system::error_code close_ec;
        sock.close(close_ec);
        if (close_ec) {
          UNILINK_LOG_DEBUG("tcp_server", "accept", "Error closing rejected socket: " + close_ec.message());
        }

        self->impl_->paused_accept_ = true;
        return;
      }
    }

    if (!ep_ec) {
      UNILINK_LOG_INFO("tcp_server", "accept", "Client connected: " + client_info);
    } else {
      UNILINK_LOG_INFO("tcp_server", "accept", "Client connected (endpoint unknown)");
    }

    auto new_session =
        std::make_shared<TcpServerSession>(self->impl_->ioc_, std::move(sock), self->impl_->cfg_.backpressure_threshold);

    size_t client_id;
    {
      std::unique_lock<std::shared_mutex> lock(self->impl_->sessions_mutex_);

      client_id = self->impl_->next_client_id_.fetch_add(1);
      self->impl_->sessions_.emplace(client_id, new_session);
      self->impl_->current_session_ = new_session;
    }

    new_session->on_bytes([self, client_id](memory::ConstByteSpan data) {
      OnBytes cb;
      MultiClientDataHandler multi_cb;
      {
        std::shared_lock<std::shared_mutex> lock(self->impl_->sessions_mutex_);
        cb = self->impl_->on_bytes_;
        multi_cb = self->impl_->on_multi_data_;
      }

      if (cb) {
        cb(data);
      }

      if (multi_cb) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        multi_cb(client_id, str_data);
      }
    });

    if (self->impl_->on_bp_) new_session->on_backpressure(self->impl_->on_bp_);

    new_session->on_close([self, client_id, new_session] {
      if (self->impl_->stopping_.load()) {
        return;
      }

      MultiClientDisconnectHandler disconnect_cb;
      {
        std::shared_lock<std::shared_mutex> lock(self->impl_->sessions_mutex_);
        disconnect_cb = self->impl_->on_multi_disconnect_;
      }

      if (disconnect_cb) {
        disconnect_cb(client_id);
      }

      bool was_current = false;
      {
        std::unique_lock<std::shared_mutex> lock(self->impl_->sessions_mutex_);
        self->impl_->sessions_.erase(client_id);

        if (self->impl_->paused_accept_ &&
            (!self->impl_->client_limit_enabled_ || self->impl_->sessions_.size() < self->impl_->max_clients_)) {
          self->impl_->paused_accept_ = false;
          net::post(self->impl_->ioc_, [self] { self->impl_->do_accept(self); });
        }

        was_current = (self->impl_->current_session_ == new_session);
        if (was_current) {
          if (!self->impl_->sessions_.empty()) {
            self->impl_->current_session_ = self->impl_->sessions_.begin()->second;
          } else {
            self->impl_->current_session_.reset();
          }
        }
      }

      if (was_current) {
        self->impl_->state_.set_state(base::LinkState::Listening);
        self->impl_->notify_state();
      }
    });

    MultiClientConnectHandler connect_cb;
    {
      std::shared_lock<std::shared_mutex> lock(self->impl_->sessions_mutex_);
      connect_cb = self->impl_->on_multi_connect_;
    }
    if (connect_cb) {
      connect_cb(client_id, client_info);
    }

    self->impl_->state_.set_state(base::LinkState::Connected);
    self->impl_->notify_state();

    new_session->start();

    self->impl_->do_accept(self);
  });
}

void TcpServer::Impl::notify_state() {
  if (stopping_.load()) return;
  OnState cb;
  {
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    if (stopping_.load()) return;
    cb = on_state_;
  }
  if (cb) {
    try {
      cb(state_.get_state());
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server", "callback", "State callback error: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_server", "callback", "Unknown error in state callback");
    }
  }
}

}  // namespace transport
}  // namespace unilink
