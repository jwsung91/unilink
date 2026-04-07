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
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/uds/uds_server.hpp"

namespace unilink {
namespace wrapper {

struct UdsServer::Impl {
  mutable std::mutex mutex_;
  std::string socket_path_;
  std::shared_ptr<transport::UdsServer> server_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::promise<bool> start_promise_;
  std::atomic<bool> started_{false};
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  ConnectionHandler client_connect_handler_{nullptr};
  ConnectionHandler client_disconnect_handler_{nullptr};
  MessageHandler data_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};

  bool auto_manage_ = false;
  size_t max_clients_ = 100;

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

  std::future<bool> start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    start_promise_ = std::promise<bool>();
    auto f = start_promise_.get_future();

    if (!server_) {
      config::UdsServerConfig cfg;
      cfg.socket_path = socket_path_;
      cfg.max_connections = static_cast<int>(max_clients_);

      if (use_external_context_) {
        server_ = std::dynamic_pointer_cast<transport::UdsServer>(factory::ChannelFactory::create(cfg, external_ioc_));
        if (manage_external_context_ && !external_thread_.joinable()) {
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

    server_->start();
    started_ = true;
    start_promise_.set_value(true);
    return f;
  }

  void stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_.exchange(false)) {
      return;
    }

    if (server_) {
      lock.unlock();
      server_->stop();
      lock.lock();
    }

    if (work_guard_) {
      work_guard_.reset();
    }

    if (external_ioc_) {
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
  }

  void setup_internal_handlers() {
    if (!server_) return;

    std::weak_ptr<bool> weak_alive = alive_marker_;

    server_->on_multi_connect([weak_alive, this](size_t client_id, const std::string& info) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      ConnectionHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = client_connect_handler_;
      }
      if (handler) {
        handler(ConnectionContext(client_id, info));
      }
    });

    server_->on_multi_disconnect([weak_alive, this](size_t client_id) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      ConnectionHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = client_disconnect_handler_;
      }
      if (handler) {
        handler(ConnectionContext(client_id, "disconnected"));
      }
    });

    server_->on_multi_data([weak_alive, this](size_t client_id, const std::vector<uint8_t>& data) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      MessageHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = data_handler_;
      }
      if (handler) {
        handler(MessageContext(client_id, std::string_view(reinterpret_cast<const char*>(data.data()), data.size())));
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

bool UdsServer::is_listening() const {
  return impl_->started_.load() && impl_->server_ && impl_->server_->is_connected();
}

bool UdsServer::broadcast(std::string_view data) {
  if (impl_->server_) {
    std::vector<uint8_t> vec(data.begin(), data.end());
    return impl_->server_->broadcast(vec);
  }
  return false;
}

bool UdsServer::send_to(size_t client_id, std::string_view data) {
  if (impl_->server_) {
    std::vector<uint8_t> vec(data.begin(), data.end());
    return impl_->server_->send_to_client(client_id, vec);
  }
  return false;
}

ServerInterface& UdsServer::on_client_connect(ConnectionHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->client_connect_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_client_disconnect(ConnectionHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->client_disconnect_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_data(MessageHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->data_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_error(ErrorHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->error_handler_ = std::move(handler);
  return *this;
}

size_t UdsServer::get_client_count() const { return impl_->server_ ? impl_->server_->get_client_count() : 0; }

std::vector<size_t> UdsServer::get_connected_clients() const {
  return impl_->server_ ? impl_->server_->get_connected_clients() : std::vector<size_t>();
}

UdsServer& UdsServer::auto_manage(bool manage) {
  impl_->auto_manage_ = manage;
  return *this;
}

UdsServer& UdsServer::set_client_limit(size_t max_clients) {
  impl_->max_clients_ = max_clients;
  return *this;
}

UdsServer& UdsServer::set_unlimited_clients() {
  impl_->max_clients_ = 1000000;
  return *this;
}

UdsServer& UdsServer::set_manage_external_context(bool manage) {
  impl_->manage_external_context_ = manage;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
