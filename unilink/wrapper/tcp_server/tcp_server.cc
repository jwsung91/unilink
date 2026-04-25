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
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {
namespace wrapper {

struct TcpServer::Impl {
  mutable std::shared_mutex mutex_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  std::atomic<bool> started_{false};
  std::atomic<bool> is_listening_{false};
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  // Configuration
  bool auto_start_{false};
  bool port_retry_enabled_{false};
  int max_port_retries_{3};
  int port_retry_interval_ms_{1000};
  int idle_timeout_ms_{0};
  bool client_limit_enabled_{false};
  size_t max_clients_{0};

  ConnectionHandler on_client_connect_{nullptr};
  ConnectionHandler on_disconnect_{nullptr};
  MessageHandler on_data_{nullptr};
  ErrorHandler on_error_{nullptr};
  FramerFactory framer_factory_{nullptr};
  MessageHandler on_message_{nullptr};

  std::unordered_map<ClientId, std::unique_ptr<framer::IFramer>> framers_;
  // Cached transport pointer — set once in start(), avoids repeated dynamic_cast.
  std::shared_ptr<transport::TcpServer> transport_cache_;

  explicit Impl(uint16_t port)
      : port_(port),
        started_(false),
        is_listening_(false),
        auto_start_(false),
        port_retry_enabled_(false),
        max_port_retries_(3),
        port_retry_interval_ms_(1000),
        idle_timeout_ms_(0),
        client_limit_enabled_(false),
        max_clients_(0) {}

  Impl(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : port_(port),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr),
        manage_external_context_(false),
        started_(false),
        is_listening_(false),
        auto_start_(false),
        port_retry_enabled_(false),
        max_port_retries_(3),
        port_retry_interval_ms_(1000),
        idle_timeout_ms_(0),
        client_limit_enabled_(false),
        max_clients_(0) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel)
      : port_(0),
        channel_(std::move(channel)),
        started_(false),
        is_listening_(false),
        auto_start_(false),
        port_retry_enabled_(false),
        max_port_retries_(3),
        port_retry_interval_ms_(1000),
        idle_timeout_ms_(0),
        client_limit_enabled_(false),
        max_clients_(0) {
    setup_internal_handlers();
  }

  ~Impl() {
    try {
      stop();
    } catch (...) {
    }
  }

  void fulfill_all_locked(bool value) {
    for (auto& p : pending_promises_) {
      try {
        p.set_value(value);
      } catch (...) {
      }
    }
    pending_promises_.clear();
  }

  std::future<bool> start() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (is_listening_.load()) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }
    std::promise<bool> p;
    auto f = p.get_future();
    pending_promises_.push_back(std::move(p));
    if (started_.exchange(true)) return f;

    if (!channel_) {
      config::TcpServerConfig config;
      config.port = port_;
      config.enable_port_retry = port_retry_enabled_;
      config.max_port_retries = max_port_retries_;
      config.port_retry_interval_ms = port_retry_interval_ms_;
      config.idle_timeout_ms = idle_timeout_ms_;

      channel_ = factory::ChannelFactory::create(config, external_ioc_);
      transport_cache_ = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
      setup_internal_handlers();

      if (client_limit_enabled_) {
        auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
        if (transport_server) {
          if (max_clients_ == 0)
            transport_server->set_unlimited_clients();
          else
            transport_server->set_client_limit(max_clients_);
        }
      }
    }
    lock.unlock();
    channel_->start();
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      if (external_ioc_ && external_ioc_->stopped()) {
        external_ioc_->restart();
      }
      work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          boost::asio::make_work_guard(*external_ioc_));
      external_thread_ = std::thread([ioc = external_ioc_]() {
        try {
          ioc->run();
        } catch (...) {
        }
      });
    }
    return f;
  }

  void stop() {
    bool should_join = false;
    {
      std::unique_lock<std::shared_mutex> lock(mutex_);
      if (!started_.exchange(false)) {
        is_listening_ = false;
        fulfill_all_locked(false);
        return;
      }
      if (channel_) {
        channel_->on_bytes(nullptr);
        channel_->on_state(nullptr);
        auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
        if (transport_server) transport_server->request_stop();
        lock.unlock();
        channel_->stop();
        lock.lock();
      }
      if (use_external_context_ && manage_external_context_) {
        if (work_guard_) work_guard_.reset();
        if (external_ioc_) external_ioc_->stop();
        should_join = true;
      }
      fulfill_all_locked(false);
      is_listening_ = false;
    }
    if (should_join && external_thread_.joinable()) {
      try {
        if (std::this_thread::get_id() != external_thread_.get_id()) {
          external_thread_.join();
        } else {
          external_thread_.detach();
        }
      } catch (...) {
      }
    }
    std::lock_guard<std::shared_mutex> lock(mutex_);
    channel_.reset();
  }

  void setup_internal_handlers() {
    if (!channel_) return;
    std::weak_ptr<bool> weak_alive = alive_marker_;
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_connect([this, weak_alive](ClientId id, const std::string& info) {
        auto alive = weak_alive.lock();
        if (!alive) return;

        ConnectionHandler handler;
        {
          std::lock_guard<std::shared_mutex> lock(mutex_);
          if (framer_factory_) {
            auto framer = framer_factory_();
            if (framer) {
              framer->on_message([this, id](memory::ConstByteSpan msg) {
                MessageHandler on_message_handler;
                {
                  std::lock_guard<std::shared_mutex> lock(mutex_);
                  on_message_handler = on_message_;
                }
                if (on_message_handler) {
                  std::string str_msg = base::safe_convert::uint8_to_string(msg.data(), msg.size());
                  on_message_handler(MessageContext(id, std::move(str_msg)));
                }
              });
              framers_[id] = std::move(framer);
            }
          }
          handler = on_client_connect_;
        }
        if (handler) handler(ConnectionContext(id, info));
      });
      transport_server->on_multi_data([this, weak_alive](ClientId id, memory::ConstByteSpan data_span) {
        auto alive = weak_alive.lock();
        if (!alive) return;

        std::string data(reinterpret_cast<const char*>(data_span.data()), data_span.size());
        MessageHandler handler;
        {
          std::shared_lock<std::shared_mutex> lock(mutex_);
          handler = on_data_;
        }
        if (handler) handler(MessageContext(id, std::move(data)));

        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = framers_.find(id);
        if (it != framers_.end()) {
          it->second->push_bytes(data_span);
        }
      });
      transport_server->on_multi_disconnect([this, weak_alive](ClientId id) {
        auto alive = weak_alive.lock();
        if (!alive) return;

        ConnectionHandler handler;
        {
          std::lock_guard<std::shared_mutex> lock(mutex_);
          framers_.erase(id);
          handler = on_disconnect_;
        }
        if (handler) handler(ConnectionContext(id));
      });
    }
    channel_->on_state([this, weak_alive](base::LinkState state) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      if (state == base::LinkState::Listening) {
        is_listening_ = true;
        std::lock_guard<std::shared_mutex> lock(mutex_);
        fulfill_all_locked(true);
      } else if (state == base::LinkState::Error || state == base::LinkState::Closed ||
                 state == base::LinkState::Idle) {
        ErrorHandler handler;
        is_listening_ = false;
        {
          std::lock_guard<std::shared_mutex> lock(mutex_);
          fulfill_all_locked(false);
          if (state == base::LinkState::Error) {
            handler = on_error_;
          }
        }
        if (handler) {
          handler(ErrorContext(ErrorCode::IoError, "Server error"));
        }
      }
    });
  }
};

TcpServer::TcpServer(uint16_t port) : impl_(std::make_unique<Impl>(port)) {}
TcpServer::TcpServer(uint16_t port, std::shared_ptr<boost::asio::io_context> ioc)
    : impl_(std::make_unique<Impl>(port, ioc)) {}
TcpServer::TcpServer(std::shared_ptr<interface::Channel> ch) : impl_(std::make_unique<Impl>(ch)) {}
TcpServer::~TcpServer() = default;

TcpServer::TcpServer(TcpServer&&) noexcept = default;
TcpServer& TcpServer::operator=(TcpServer&&) noexcept = default;

std::future<bool> TcpServer::start() { return impl_->start(); }
void TcpServer::stop() { impl_->stop(); }
bool TcpServer::listening() const { return get_impl()->is_listening_.load(); }

bool TcpServer::broadcast(std::string_view data) {
  const auto& ts = impl_->transport_cache_;
  return ts ? ts->broadcast(data) : false;
}

bool TcpServer::send_to(ClientId client_id, std::string_view data) {
  const auto& ts = impl_->transport_cache_;
  return ts ? ts->send_to_client(client_id, data) : false;
}

ServerInterface& TcpServer::on_connect(ConnectionHandler h) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_client_connect_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_disconnect(ConnectionHandler h) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_disconnect_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_data(MessageHandler h) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_data_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_error(ErrorHandler h) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_error_ = std::move(h);
  return *this;
}

ServerInterface& TcpServer::framer(FramerFactory factory) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->framer_factory_ = std::move(factory);
  return *this;
}

ServerInterface& TcpServer::on_message(MessageHandler handler) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_message_ = std::move(handler);
  return *this;
}

size_t TcpServer::client_count() const {
  const auto& ts = get_impl()->transport_cache_;
  return ts ? ts->client_count() : 0;
}

std::vector<ClientId> TcpServer::connected_clients() const {
  const auto& ts = get_impl()->transport_cache_;
  return ts ? ts->connected_clients() : std::vector<ClientId>();
}

TcpServer& TcpServer::auto_start(bool m) {
  impl_->auto_start_ = m;
  if (impl_->auto_start_ && !impl_->started_.load()) start();
  return *this;
}

TcpServer& TcpServer::port_retry(bool e, int m, int i) {
  impl_->port_retry_enabled_ = e;
  impl_->max_port_retries_ = m;
  impl_->port_retry_interval_ms_ = i;
  return *this;
}

TcpServer& TcpServer::idle_timeout(std::chrono::milliseconds timeout) {
  impl_->idle_timeout_ms_ = static_cast<int>(timeout.count());
  // Note: Runtime change of idle_timeout is not supported by current transport.
  // Must be set via Builder before start().
  return *this;
}

TcpServer& TcpServer::max_clients(size_t max) {
  impl_->max_clients_ = max;
  impl_->client_limit_enabled_ = true;
  if (impl_->transport_cache_) impl_->transport_cache_->set_client_limit(max);
  return *this;
}

TcpServer& TcpServer::unlimited_clients() {
  impl_->client_limit_enabled_ = false;
  if (impl_->transport_cache_) impl_->transport_cache_->set_unlimited_clients();
  return *this;
}

TcpServer& TcpServer::manage_external_context(bool m) {
  impl_->manage_external_context_ = m;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
