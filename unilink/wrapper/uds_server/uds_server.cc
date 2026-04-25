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

#include "unilink/wrapper/uds_server/uds_server.hpp"

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/uds/uds_server.hpp"

namespace unilink {
namespace wrapper {

struct UdsServer::Impl {
  mutable std::shared_mutex mutex_;
  std::string socket_path_;
  std::shared_ptr<transport::UdsServer> server_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  std::atomic<bool> started_{false};
  std::atomic<bool> is_listening_{false};
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  ConnectionHandler client_connect_handler_{nullptr};
  ConnectionHandler client_disconnect_handler_{nullptr};
  MessageHandler data_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};
  FramerFactory framer_factory_{nullptr};
  MessageHandler on_message_{nullptr};

  std::unordered_map<ClientId, std::unique_ptr<framer::IFramer>> framers_;

  bool auto_start_ = false;
  size_t max_clients_ = 100;
  int idle_timeout_ms_ = 0;

  explicit Impl(const std::string& socket_path) : socket_path_(socket_path), started_(false) {}

  Impl(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> external_ioc)
      : socket_path_(socket_path),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr),
        manage_external_context_(false),
        started_(false) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel)
      : socket_path_(""), server_(std::dynamic_pointer_cast<transport::UdsServer>(channel)), started_(false) {
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
    pending_promises_.emplace_back(std::move(p));

    if (started_.exchange(true)) {
      return f;
    }

    if (!server_) {
      config::UdsServerConfig cfg;
      cfg.socket_path = socket_path_;
      cfg.max_connections = static_cast<int>(max_clients_);
      cfg.idle_timeout_ms = idle_timeout_ms_;

      if (use_external_context_) {
        server_ = std::dynamic_pointer_cast<transport::UdsServer>(factory::ChannelFactory::create(cfg, external_ioc_));
        if (manage_external_context_ && !external_thread_.joinable()) {
          if (external_ioc_ && external_ioc_->stopped()) {
            external_ioc_->restart();
          }
          work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
              boost::asio::make_work_guard(*external_ioc_));
          external_thread_ = std::thread([this]() {
            try {
              external_ioc_->run();
            } catch (...) {
            }
          });
        }
      } else {
        server_ = std::dynamic_pointer_cast<transport::UdsServer>(factory::ChannelFactory::create(cfg));
      }
      setup_internal_handlers();
    }

    lock.unlock();
    server_->start();
    return f;
  }

  void stop() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!started_.exchange(false)) {
      fulfill_all_locked(false);
      return;
    }

    if (server_) {
      server_->on_multi_connect(nullptr);
      server_->on_multi_data(nullptr);
      server_->on_multi_disconnect(nullptr);
      server_->on_bytes(nullptr);
      server_->on_state(nullptr);
      server_->on_backpressure(nullptr);
      lock.unlock();
      server_->stop();
      lock.lock();
    }

    if (work_guard_) {
      work_guard_.reset();
    }

    if (use_external_context_ && manage_external_context_ && external_ioc_) {
      external_ioc_->stop();
    }

    if (external_thread_.joinable()) {
      if (std::this_thread::get_id() != external_thread_.get_id()) {
        lock.unlock();  // RELEASE LOCK BEFORE JOINING
        external_thread_.join();
        lock.lock();  // RE-ACQUIRE
      } else {
        external_thread_.detach();
      }
    }

    is_listening_ = false;
    fulfill_all_locked(false);
    server_.reset();
  }

  void setup_internal_handlers() {
    if (!server_) return;

    std::weak_ptr<bool> weak_alive = alive_marker_;

    server_->on_state([weak_alive, this](base::LinkState state) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      ErrorHandler error_handler;
      {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        if (state == base::LinkState::Listening || state == base::LinkState::Connected) {
          is_listening_ = true;
          fulfill_all_locked(true);
        } else if (state == base::LinkState::Error || state == base::LinkState::Closed ||
                   state == base::LinkState::Idle) {
          is_listening_ = false;
          fulfill_all_locked(false);
          if (state == base::LinkState::Error) {
            error_handler = error_handler_;
          }
        }
      }

      if (error_handler) {
        error_handler(ErrorContext(ErrorCode::IoError, "Server error"));
      }
    });

    server_->on_multi_connect([weak_alive, this](ClientId client_id, const std::string& info) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        if (framer_factory_) {
          auto framer = framer_factory_();
          if (framer) {
            framer->on_message([this, client_id](memory::ConstByteSpan msg) {
              MessageHandler on_message_handler;
              {
                std::lock_guard<std::shared_mutex> lock(mutex_);
                on_message_handler = on_message_;
              }
              if (on_message_handler) {
                std::string str_msg = base::safe_convert::uint8_to_string(msg.data(), msg.size());
                on_message_handler(MessageContext(client_id, std::move(str_msg)));
              }
            });
            framers_[client_id] = std::move(framer);
          }
        }
      }

      ConnectionHandler handler;
      {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        handler = client_connect_handler_;
      }
      if (handler) {
        handler(ConnectionContext(client_id, info));
      }
    });

    server_->on_multi_disconnect([weak_alive, this](ClientId client_id) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        framers_.erase(client_id);
      }

      ConnectionHandler handler;
      {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        handler = client_disconnect_handler_;
      }
      if (handler) {
        handler(ConnectionContext(client_id, "disconnected"));
      }
    });

    server_->on_multi_data([weak_alive, this](ClientId client_id, memory::ConstByteSpan data_span) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      // 1. Raw data handler
      MessageHandler handler;
      {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        handler = data_handler_;
      }
      if (handler) {
        std::string str_data = base::safe_convert::uint8_to_string(data_span.data(), data_span.size());
        handler(MessageContext(client_id, std::move(str_data)));
      }

      // 2. Framer integration
      {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = framers_.find(client_id);
        if (it != framers_.end()) {
          it->second->push_bytes(data_span);
        }
      }
    });
  }
};

UdsServer::UdsServer(const std::string& socket_path) : impl_(std::make_unique<Impl>(socket_path)) {}

UdsServer::UdsServer(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> external_ioc)
    : impl_(std::make_unique<Impl>(socket_path, std::move(external_ioc))) {}

UdsServer::UdsServer(std::shared_ptr<interface::Channel> channel) : impl_(std::make_unique<Impl>(std::move(channel))) {}

UdsServer::~UdsServer() = default;

UdsServer::UdsServer(UdsServer&&) noexcept = default;
UdsServer& UdsServer::operator=(UdsServer&&) noexcept = default;

std::future<bool> UdsServer::start() { return impl_->start(); }

void UdsServer::stop() { impl_->stop(); }

bool UdsServer::listening() const { return impl_->is_listening_.load(); }

bool UdsServer::broadcast(std::string_view data) { return impl_->server_ ? impl_->server_->broadcast(data) : false; }

bool UdsServer::send_to(ClientId client_id, std::string_view data) {
  return impl_->server_ ? impl_->server_->send_to_client(client_id, data) : false;
}

ServerInterface& UdsServer::on_connect(ConnectionHandler handler) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->client_connect_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_disconnect(ConnectionHandler handler) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->client_disconnect_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_data(MessageHandler handler) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_error(ErrorHandler handler) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->error_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::framer(FramerFactory factory) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->framer_factory_ = std::move(factory);
  return *this;
}

ServerInterface& UdsServer::on_message(MessageHandler handler) {
  std::lock_guard<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_message_ = std::move(handler);
  return *this;
}

size_t UdsServer::client_count() const { return impl_->server_ ? impl_->server_->client_count() : 0; }

std::vector<ClientId> UdsServer::connected_clients() const {
  return impl_->server_ ? impl_->server_->connected_clients() : std::vector<ClientId>();
}

UdsServer& UdsServer::auto_start(bool manage) {
  impl_->auto_start_ = manage;
  if (impl_->auto_start_ && !impl_->started_.load()) {
    start();
  }
  return *this;
}

UdsServer& UdsServer::idle_timeout(std::chrono::milliseconds timeout) {
  impl_->idle_timeout_ms_ = static_cast<int>(timeout.count());
  return *this;
}

UdsServer& UdsServer::max_clients(size_t max) {
  impl_->max_clients_ = max;
  return *this;
}

UdsServer& UdsServer::unlimited_clients() {
  impl_->max_clients_ = 1000000;
  return *this;
}

UdsServer& UdsServer::manage_external_context(bool manage) {
  impl_->manage_external_context_ = manage;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
