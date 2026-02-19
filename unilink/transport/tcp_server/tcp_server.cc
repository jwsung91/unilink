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

#include <atomic>
#include <future>
#include <iostream>

#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/transport/tcp_server/boost_tcp_acceptor.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

std::shared_ptr<TcpServer> TcpServer::create(const config::TcpServerConfig& cfg) {
  return std::shared_ptr<TcpServer>(new TcpServer(cfg));
}

std::shared_ptr<TcpServer> TcpServer::create(const config::TcpServerConfig& cfg,
                                             std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                                             net::io_context& ioc) {
  return std::shared_ptr<TcpServer>(new TcpServer(cfg, std::move(acceptor), ioc));
}

TcpServer::TcpServer(const config::TcpServerConfig& cfg)
    : stopping_(false),
      next_client_id_(0),
      owned_ioc_(nullptr),
      owns_ioc_(false),
      uses_global_ioc_(true),
      ioc_(concurrency::IoContextManager::instance().get_context()),
      acceptor_(nullptr),
      cfg_(cfg),
      max_clients_(cfg.max_connections > 0 ? static_cast<size_t>(cfg.max_connections) : 0),
      client_limit_enabled_(cfg.max_connections > 0),
      paused_accept_(false),
      current_session_(nullptr) {
  // Create acceptor after all members are initialized
  try {
    acceptor_ = std::make_unique<BoostTcpAcceptor>(ioc_);
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to create TCP acceptor: " + std::string(e.what()));
  }
}

TcpServer::TcpServer(const config::TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                     net::io_context& ioc)
    : stopping_(false),
      next_client_id_(0),
      owned_ioc_(nullptr),
      owns_ioc_(false),
      uses_global_ioc_(false),
      ioc_(ioc),
      acceptor_(std::move(acceptor)),
      cfg_(cfg),
      max_clients_(cfg.max_connections > 0 ? static_cast<size_t>(cfg.max_connections) : 0),
      client_limit_enabled_(cfg.max_connections > 0),
      paused_accept_(false),
      current_session_(nullptr) {
  // Ensure acceptor is properly initialized
  if (!acceptor_) {
    throw std::runtime_error("Failed to create TCP acceptor");
  }
}

TcpServer::~TcpServer() {
  try {
    // Ensure proper cleanup even if stop() wasn't called explicitly
    if (!state_.is_state(base::LinkState::Closed)) {
      stop();
    }
  } catch (...) {
    // Prevent exceptions from escaping destructor
    std::cerr << "TcpServer destructor failed" << std::endl;
  }
}

void TcpServer::start() {
  auto current = state_.get_state();
  if (current == base::LinkState::Listening || current == base::LinkState::Connected ||
      current == base::LinkState::Connecting) {
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
    state_.set_state(base::LinkState::Error);
    notify_state();
    return;
  }

  if (owns_ioc_) {
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }
  auto self = shared_from_this();
  if (ioc_.get_executor().running_in_this_thread()) {
    if (!self->stopping_.load()) {
      self->attempt_port_binding(0);
    }
  } else {
    net::dispatch(ioc_, [self] {
      if (self->stopping_.load()) return;
      self->attempt_port_binding(0);
    });
  }
}

void TcpServer::stop() {
  if (stopping_.exchange(true)) {
    return;
  }

  try {
    {
      std::lock_guard<std::mutex> lock(sessions_mutex_);
      on_bytes_ = nullptr;
      on_state_ = nullptr;
      on_bp_ = nullptr;
      on_multi_connect_ = nullptr;
      on_multi_data_ = nullptr;
      on_multi_disconnect_ = nullptr;
    }

    auto cleanup_flag = std::make_shared<std::atomic<bool>>(false);
    auto cleanup = [this, cleanup_flag]() {
      if (cleanup_flag->exchange(true)) return;

      try {
        boost::system::error_code ec;
        if (acceptor_ && acceptor_->is_open()) {
          acceptor_->close(ec);
        }

        std::vector<std::shared_ptr<TcpServerSession>> sessions_copy;
        {
          std::lock_guard<std::mutex> lock(sessions_mutex_);
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
        notify_state();
      } catch (const std::exception& e) {
        // Prevent exceptions from escaping cleanup, especially when called from destructor
        std::cerr << "TcpServer cleanup failed: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "TcpServer cleanup failed with unknown error" << std::endl;
      }
    };

    const bool in_ioc_thread = ioc_.get_executor().running_in_this_thread();

    if (in_ioc_thread) {
      cleanup();
      if (owns_ioc_) {
        ioc_.stop();
      }
      return;
    }

    bool use_sync_cleanup = true;
    const bool has_active_ioc_thread =
        owns_ioc_ || (uses_global_ioc_ && concurrency::IoContextManager::instance().is_running());

    if (has_active_ioc_thread) {
      try {
        auto self = shared_from_this();
        use_sync_cleanup = false;

        auto cleanup_promise = std::make_shared<std::promise<void>>();
        auto cleanup_future = cleanup_promise->get_future();

        // Dispatch with WEAK pointer to avoid keeping IoContext alive during static destruction
        std::weak_ptr<TcpServer> weak_self = self;
        net::dispatch(ioc_, [weak_self, cleanup, cleanup_promise]() {
          // Only run cleanup if TcpServer is still alive
          if (auto shared_self = weak_self.lock()) {
            cleanup();
          }
          cleanup_promise->set_value();
        });

        if (cleanup_future.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
          UNILINK_LOG_WARNING("tcp_server", "stop", "Async stop timed out, continuing in background");
        }
      } catch (const std::bad_weak_ptr&) {
        use_sync_cleanup = true;
      } catch (...) {
        use_sync_cleanup = true;
      }
    }

    if (use_sync_cleanup) {
      cleanup();
    }

    if (owns_ioc_ && ioc_thread_.joinable()) {
      ioc_thread_.join();
      ioc_.restart();
    }
  } catch (const std::exception& e) {
    std::cerr << "TcpServer::stop failed: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "TcpServer::stop failed with unknown error" << std::endl;
  }
}

void TcpServer::request_stop() {
  if (stopping_.load()) return;
  auto self = shared_from_this();
  net::post(ioc_, [self] { self->stop(); });
}

bool TcpServer::is_connected() const {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  return current_session_ && current_session_->alive();
}

void TcpServer::async_write_copy(memory::ConstByteSpan data) {
  if (stopping_.load()) return;
  std::shared_ptr<TcpServerSession> session;
  {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    session = current_session_;
  }

  if (session && session->alive()) {
    session->async_write_copy(data);
  }
}

void TcpServer::async_write_move(std::vector<uint8_t>&& data) {
  if (stopping_.load()) return;
  std::shared_ptr<TcpServerSession> session;
  {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    session = current_session_;
  }

  if (session && session->alive()) {
    session->async_write_move(std::move(data));
  }
}

void TcpServer::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (stopping_.load() || !data) return;
  std::shared_ptr<TcpServerSession> session;
  {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    session = current_session_;
  }

  if (session && session->alive()) {
    session->async_write_shared(std::move(data));
  }
}

void TcpServer::on_bytes(OnBytes cb) {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  on_bytes_ = std::move(cb);
}
void TcpServer::on_state(OnState cb) {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  on_state_ = std::move(cb);
}
void TcpServer::on_backpressure(OnBackpressure cb) {
  {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    on_bp_ = std::move(cb);
  }
  std::shared_ptr<TcpServerSession> session;
  {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    session = current_session_;
  }

  if (session) session->on_backpressure(on_bp_);
}

void TcpServer::attempt_port_binding(int retry_count) {
  if (stopping_.load()) return;
  boost::system::error_code ec;

  if (!acceptor_->is_open()) {
    acceptor_->open(tcp::v4(), ec);
    if (ec) {
      state_.set_state(base::LinkState::Error);
      notify_state();
      return;
    }
  }

  acceptor_->bind(tcp::endpoint(tcp::v4(), cfg_.port), ec);
  if (ec) {
    if (cfg_.enable_port_retry && retry_count < cfg_.max_port_retries) {
      auto self = shared_from_this();
      auto timer = std::make_shared<net::steady_timer>(ioc_);
      timer->expires_after(std::chrono::milliseconds(cfg_.port_retry_interval_ms));
      timer->async_wait([self, retry_count, timer](const boost::system::error_code& timer_ec) {
        if (!timer_ec) {
          self->attempt_port_binding(retry_count + 1);
        }
      });
      return;
    } else {
      state_.set_state(base::LinkState::Error);
      notify_state();
      return;
    }
  }

  acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    state_.set_state(base::LinkState::Error);
    notify_state();
    return;
  }

  state_.set_state(base::LinkState::Listening);
  notify_state();
  do_accept();
}

void TcpServer::do_accept() {
  if (stopping_.load() || !acceptor_ || !acceptor_->is_open()) return;

  auto self = shared_from_this();
  acceptor_->async_accept([self](auto ec, tcp::socket sock) {
    if (self->stopping_.load()) {
      return;
    }
    if (ec) {
      if (ec != boost::asio::error::operation_aborted) {
        self->state_.set_state(base::LinkState::Error);
        self->notify_state();
      }
      if (!self->state_.is_state(base::LinkState::Closed) && !self->stopping_.load()) {
        auto timer = std::make_shared<net::steady_timer>(self->ioc_);
        timer->expires_after(std::chrono::milliseconds(100));
        timer->async_wait([self, timer](const boost::system::error_code&) {
          if (!self->stopping_.load()) {
            self->do_accept();
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

    if (self->client_limit_enabled_) {
      std::lock_guard<std::mutex> lock(self->sessions_mutex_);
      if (self->sessions_.size() >= self->max_clients_) {
        boost::system::error_code close_ec;
        sock.close(close_ec);
        self->paused_accept_ = true;
        return;
      }
    }

    auto new_session = std::make_shared<TcpServerSession>(
        self->ioc_, std::move(sock), self->cfg_.backpressure_threshold, self->cfg_.idle_timeout_ms);

    size_t client_id;
    {
      std::lock_guard<std::mutex> lock(self->sessions_mutex_);
      client_id = self->next_client_id_.fetch_add(1);
      self->sessions_.emplace(client_id, new_session);
      self->current_session_ = new_session;
    }

    new_session->on_bytes([self, client_id](memory::ConstByteSpan data) {
      OnBytes cb;
      MultiClientDataHandler multi_cb;
      {
        std::lock_guard<std::mutex> lock(self->sessions_mutex_);
        cb = self->on_bytes_;
        multi_cb = self->on_multi_data_;
      }
      if (cb) cb(data);
      if (multi_cb) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        multi_cb(client_id, str_data);
      }
    });

    if (self->on_bp_) new_session->on_backpressure(self->on_bp_);

    new_session->on_close([self, client_id, new_session] {
      if (self->stopping_.load()) return;
      MultiClientDisconnectHandler disconnect_cb;
      {
        std::lock_guard<std::mutex> lock(self->sessions_mutex_);
        disconnect_cb = self->on_multi_disconnect_;
      }
      if (disconnect_cb) disconnect_cb(client_id);

      bool was_current = false;
      {
        std::lock_guard<std::mutex> lock(self->sessions_mutex_);
        self->sessions_.erase(client_id);
        if (self->paused_accept_ && (!self->client_limit_enabled_ || self->sessions_.size() < self->max_clients_)) {
          self->paused_accept_ = false;
          net::post(self->ioc_, [self] { self->do_accept(); });
        }
        was_current = (self->current_session_ == new_session);
        if (was_current) {
          if (!self->sessions_.empty())
            self->current_session_ = self->sessions_.begin()->second;
          else
            self->current_session_.reset();
        }
      }
      if (was_current) {
        self->state_.set_state(base::LinkState::Listening);
        self->notify_state();
      }
    });

    MultiClientConnectHandler connect_cb;
    {
      std::lock_guard<std::mutex> lock(self->sessions_mutex_);
      connect_cb = self->on_multi_connect_;
    }
    if (connect_cb) connect_cb(client_id, client_info);

    self->state_.set_state(base::LinkState::Connected);
    self->notify_state();
    new_session->start();
    self->do_accept();
  });
}

void TcpServer::notify_state() {
  if (stopping_.load()) return;
  OnState cb;
  try {
    {
      std::lock_guard<std::mutex> lock(sessions_mutex_);
      cb = on_state_;
    }
    if (cb) {
      cb(state_.get_state());
    }
  } catch (...) {
  }
}

bool TcpServer::broadcast(const std::string& message) {
  auto shared_data = std::make_shared<const std::vector<uint8_t>>(message.begin(), message.end());
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  bool sent = false;
  for (auto& entry : sessions_) {
    auto& session = entry.second;
    if (session && session->alive()) {
      session->async_write_shared(shared_data);
      sent = true;
    }
  }
  return sent;
}

bool TcpServer::send_to_client(size_t client_id, const std::string& message) {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  auto it = sessions_.find(client_id);
  if (it != sessions_.end() && it->second && it->second->alive()) {
    auto binary_view = common::safe_convert::string_to_bytes(message);
    it->second->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    return true;
  }
  return false;
}

size_t TcpServer::get_client_count() const {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  size_t alive = 0;
  for (const auto& entry : sessions_)
    if (entry.second && entry.second->alive()) ++alive;
  return alive;
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  std::vector<size_t> connected_clients;
  for (const auto& entry : sessions_)
    if (entry.second && entry.second->alive()) connected_clients.push_back(entry.first);
  return connected_clients;
}

void TcpServer::on_multi_connect(MultiClientConnectHandler h) {
  std::lock_guard<std::mutex> l(sessions_mutex_);
  on_multi_connect_ = std::move(h);
}
void TcpServer::on_multi_data(MultiClientDataHandler h) {
  std::lock_guard<std::mutex> l(sessions_mutex_);
  on_multi_data_ = std::move(h);
}
void TcpServer::on_multi_disconnect(MultiClientDisconnectHandler h) {
  std::lock_guard<std::mutex> l(sessions_mutex_);
  on_multi_disconnect_ = std::move(h);
}

void TcpServer::set_client_limit(size_t max) {
  std::lock_guard<std::mutex> l(sessions_mutex_);
  max_clients_ = max;
  client_limit_enabled_ = true;
  if (paused_accept_ && sessions_.size() < max_clients_) {
    paused_accept_ = false;
    net::post(ioc_, [self = shared_from_this()] { self->do_accept(); });
  }
}

void TcpServer::set_unlimited_clients() {
  std::lock_guard<std::mutex> l(sessions_mutex_);
  client_limit_enabled_ = false;
  max_clients_ = 0;
  if (paused_accept_) {
    paused_accept_ = false;
    net::post(ioc_, [self = shared_from_this()] { self->do_accept(); });
  }
}

base::LinkState TcpServer::get_state() const { return state_.get_state(); }

}  // namespace transport
}  // namespace unilink
