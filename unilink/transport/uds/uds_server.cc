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

#include "unilink/transport/uds/uds_server.hpp"

#include <atomic>
#include <boost/asio.hpp>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/concurrency/io_context_manager.hpp"
#include "unilink/concurrency/thread_safe_state.hpp"
#include "unilink/diagnostics/logger.hpp"
#include "unilink/interface/iuds_acceptor.hpp"
#include "unilink/transport/uds/boost_uds_acceptor.hpp"
#include "unilink/transport/uds/uds_server_session.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using uds = net::local::stream_protocol;

struct UdsServer::Impl {
  std::unique_ptr<net::io_context> owned_ioc_;
  net::io_context* ioc_ = nullptr;
  std::unique_ptr<net::executor_work_guard<net::io_context::executor_type>> work_guard_;
  std::thread ioc_thread_;
  bool owns_ioc_ = true;

  std::atomic<bool> stopping_{false};
  std::atomic<size_t> next_client_id_{0};

  std::unique_ptr<interface::UdsAcceptorInterface> acceptor_;
  config::UdsServerConfig cfg_;

  concurrency::ThreadSafeLinkState state_{base::LinkState::Idle};
  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_bp_;
  MultiClientConnectHandler on_multi_connect_;
  MultiClientDataHandler on_multi_data_;
  MultiClientDisconnectHandler on_multi_disconnect_;

  mutable std::mutex sessions_mutex_;
  std::unordered_map<size_t, std::shared_ptr<UdsServerSession>> sessions_;

  Impl(const config::UdsServerConfig& cfg, net::io_context* ioc_ptr)
      : owned_ioc_(ioc_ptr ? nullptr : std::make_unique<net::io_context>()),
        ioc_(ioc_ptr ? ioc_ptr : owned_ioc_.get()),
        owns_ioc_(!ioc_ptr),
        cfg_(cfg) {
    acceptor_ = std::make_unique<BoostUdsAcceptor>(*ioc_);
  }
  ~Impl() {
    stopping_ = true;
    if (work_guard_) {
      work_guard_.reset();
    }
    if (ioc_ && owns_ioc_ && ioc_thread_.joinable()) {
      if (std::this_thread::get_id() != ioc_thread_.get_id()) {
        ioc_thread_.join();
      } else {
        ioc_thread_.detach();
      }
    }
    // UDS Cleanup: socket file should be removed.
    std::remove(cfg_.socket_path.c_str());
  }
  void do_accept(std::shared_ptr<UdsServer> self);
  void notify_state();
};

std::shared_ptr<UdsServer> UdsServer::create(const config::UdsServerConfig& cfg) {
  return std::shared_ptr<UdsServer>(new UdsServer(cfg));
}

std::shared_ptr<UdsServer> UdsServer::create(const config::UdsServerConfig& cfg,
                                             std::unique_ptr<interface::UdsAcceptorInterface> acceptor,
                                             net::io_context& ioc) {
  return std::shared_ptr<UdsServer>(new UdsServer(cfg, std::move(acceptor), ioc));
}

UdsServer::UdsServer(const config::UdsServerConfig& cfg) : impl_(std::make_unique<Impl>(cfg, nullptr)) {}
UdsServer::UdsServer(const config::UdsServerConfig& cfg, std::unique_ptr<interface::UdsAcceptorInterface> acceptor,
                     net::io_context& ioc)
    : impl_(std::make_unique<Impl>(cfg, &ioc)) {
  impl_->acceptor_ = std::move(acceptor);
}

UdsServer::~UdsServer() { stop(); }

UdsServer::UdsServer(UdsServer&&) noexcept = default;
UdsServer& UdsServer::operator=(UdsServer&&) noexcept = default;

void UdsServer::start() {
  if (impl_->state_.get_state() == base::LinkState::Listening) return;

  impl_->stopping_ = false;

  // Cleanup old socket file if exists
  std::remove(impl_->cfg_.socket_path.c_str());

  boost::system::error_code ec;
  impl_->acceptor_->open(uds(), ec);
  if (ec) {
    UNILINK_LOG_ERROR("uds_server", "start", "Failed to open acceptor: " + ec.message());
    impl_->state_.set_state(base::LinkState::Error);
    impl_->notify_state();
    return;
  }

  impl_->acceptor_->bind(uds::endpoint(impl_->cfg_.socket_path), ec);
  if (ec) {
    UNILINK_LOG_ERROR("uds_server", "start", "Failed to bind to " + impl_->cfg_.socket_path + ": " + ec.message());
    impl_->state_.set_state(base::LinkState::Error);
    impl_->notify_state();
    return;
  }

  impl_->acceptor_->listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    UNILINK_LOG_ERROR("uds_server", "start", "Failed to listen: " + ec.message());
    impl_->state_.set_state(base::LinkState::Error);
    impl_->notify_state();
    return;
  }

  impl_->state_.set_state(base::LinkState::Listening);
  impl_->notify_state();

  if (impl_->owns_ioc_ && !impl_->ioc_thread_.joinable()) {
    if (impl_->ioc_->stopped()) {
      impl_->ioc_->restart();
    }
    impl_->work_guard_ =
        std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(net::make_work_guard(*impl_->ioc_));
    impl_->ioc_thread_ = std::thread([this]() {
      try {
        impl_->ioc_->run();
      } catch (...) {
      }
    });
  }

  net::post(impl_->ioc_->get_executor(), [self = shared_from_this()]() { self->impl_->do_accept(self); });
}

void UdsServer::stop() {
  bool already_stopping = impl_->stopping_.exchange(true);
  if (already_stopping) return;

  boost::system::error_code ec;
  impl_->acceptor_->close(ec);

  {
    std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
    impl_->on_bytes_ = nullptr;
    impl_->on_state_ = nullptr;
    impl_->on_bp_ = nullptr;
    impl_->on_multi_connect_ = nullptr;
    impl_->on_multi_data_ = nullptr;
    impl_->on_multi_disconnect_ = nullptr;
  }

  // Cleanup UDS socket file on stop
  std::remove(impl_->cfg_.socket_path.c_str());

  // Release work guard if owned
  if (impl_->owns_ioc_) {
    impl_->work_guard_.reset();
  }

  std::vector<std::shared_ptr<UdsServerSession>> sessions_to_stop;
  {
    std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
    for (auto& pair : impl_->sessions_) {
      sessions_to_stop.push_back(pair.second);
    }
    impl_->sessions_.clear();
  }

  for (auto& session : sessions_to_stop) {
    session->stop();
  }

  impl_->state_.set_state(base::LinkState::Idle);
  impl_->notify_state();
}

bool UdsServer::is_connected() const { return impl_->state_.get_state() == base::LinkState::Listening; }

void UdsServer::async_write_copy(memory::ConstByteSpan data) {
  broadcast(std::vector<uint8_t>(data.begin(), data.end()));
}

void UdsServer::async_write_move(std::vector<uint8_t>&& data) { broadcast(data); }

void UdsServer::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  for (auto& pair : impl_->sessions_) {
    pair.second->async_write_shared(data);
  }
}

void UdsServer::on_bytes(OnBytes cb) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  impl_->on_bytes_ = std::move(cb);
}

void UdsServer::on_state(OnState cb) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  impl_->on_state_ = std::move(cb);
}

void UdsServer::on_backpressure(OnBackpressure cb) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  impl_->on_bp_ = std::move(cb);
}

bool UdsServer::broadcast(const std::vector<uint8_t>& message) {
  auto data = std::make_shared<const std::vector<uint8_t>>(message);
  async_write_shared(data);
  return true;
}

bool UdsServer::send_to_client(size_t client_id, const std::vector<uint8_t>& message) {
  std::shared_ptr<UdsServerSession> session;
  {
    std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
    auto it = impl_->sessions_.find(client_id);
    if (it != impl_->sessions_.end()) session = it->second;
  }
  if (session) {
    session->async_write_copy(memory::ConstByteSpan(message.data(), message.size()));
    return true;
  }
  return false;
}

size_t UdsServer::get_client_count() const {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  return impl_->sessions_.size();
}

std::vector<size_t> UdsServer::get_connected_clients() const {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  std::vector<size_t> ids;
  for (const auto& pair : impl_->sessions_) ids.push_back(pair.first);
  return ids;
}

void UdsServer::on_multi_connect(MultiClientConnectHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  impl_->on_multi_connect_ = std::move(handler);
}

void UdsServer::on_multi_data(MultiClientDataHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  impl_->on_multi_data_ = std::move(handler);
}

void UdsServer::on_multi_disconnect(MultiClientDisconnectHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->sessions_mutex_);
  impl_->on_multi_disconnect_ = std::move(handler);
}

base::LinkState UdsServer::get_state() const { return impl_->state_.get_state(); }

void UdsServer::Impl::do_accept(std::shared_ptr<UdsServer> self) {
  acceptor_->async_accept([self](const boost::system::error_code& ec, uds::socket socket) {
    if (!self || self->impl_->stopping_) return;

    if (!ec) {
      size_t client_id;
      auto session = std::make_shared<UdsServerSession>(*self->impl_->ioc_, std::move(socket),
                                                        self->impl_->cfg_.backpressure_threshold);

      {
        std::lock_guard<std::mutex> lock(self->impl_->sessions_mutex_);
        client_id = self->impl_->next_client_id_++;
        self->impl_->sessions_[client_id] = session;
      }

      std::weak_ptr<UdsServer> weak_self = self;
      session->on_bytes([weak_self, client_id](memory::ConstByteSpan data) {
        auto s = weak_self.lock();
        if (!s) return;
        MultiClientDataHandler data_handler;
        OnBytes bytes_handler;
        {
          std::lock_guard<std::mutex> lock(s->impl_->sessions_mutex_);
          data_handler = s->impl_->on_multi_data_;
          bytes_handler = s->impl_->on_bytes_;
        }
        if (data_handler) data_handler(client_id, std::vector<uint8_t>(data.begin(), data.end()));
        if (bytes_handler) bytes_handler(data);
      });

      session->on_close([weak_self, client_id]() {
        auto s = weak_self.lock();
        if (!s) return;
        MultiClientDisconnectHandler disconnect_handler;
        {
          std::lock_guard<std::mutex> lock(s->impl_->sessions_mutex_);
          s->impl_->sessions_.erase(client_id);
          disconnect_handler = s->impl_->on_multi_disconnect_;
        }
        if (disconnect_handler) disconnect_handler(client_id);
      });

      session->start();

      MultiClientConnectHandler connect_handler;
      {
        std::lock_guard<std::mutex> lock(self->impl_->sessions_mutex_);
        connect_handler = self->impl_->on_multi_connect_;
      }
      if (connect_handler) connect_handler(client_id, "UDS Client");

      // Continue accepting
      auto* impl = self->impl_.get();
      impl->do_accept(self);
    } else {
      // Log only real errors, not operation_aborted
      if (ec != boost::asio::error::operation_aborted) {
        UNILINK_LOG_ERROR("uds_server", "accept", "Accept failed: " + ec.message());
      }
    }
  });
}

void UdsServer::Impl::notify_state() {
  OnState cb;
  {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    cb = on_state_;
  }
  if (cb) cb(state_.get_state());
}

}  // namespace transport
}  // namespace unilink
