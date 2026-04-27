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
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
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
  std::shared_ptr<interface::Channel> server_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  std::atomic<bool> use_external_context_{false};
  std::atomic<bool> manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  std::atomic<bool> started_{false};
  std::atomic<bool> is_listening_{false};
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  // Configuration
  std::atomic<bool> auto_start_{false};
  std::atomic<int> idle_timeout_ms_{0};
  std::atomic<size_t> max_clients_{1000000};
  std::atomic<size_t> backpressure_threshold_{base::constants::DEFAULT_BACKPRESSURE_THRESHOLD};
  std::atomic<base::constants::BackpressureStrategy> backpressure_strategy_{
      base::constants::BackpressureStrategy::KeepAll};

  ConnectionHandler client_connect_handler_{nullptr};
  ConnectionHandler client_disconnect_handler_{nullptr};
  MessageHandler data_handler_{nullptr};
  BatchMessageHandler data_batch_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};
  FramerFactory framer_factory_{nullptr};
  MessageHandler on_message_{nullptr};
  BatchMessageHandler on_message_batch_{nullptr};

  std::unordered_map<ClientId, std::unique_ptr<framer::IFramer>> framers_;

  // Batching logic
  std::vector<MessageContext> data_batch_queue_;
  std::vector<MessageContext> message_batch_queue_;
  std::unique_ptr<boost::asio::steady_timer> batch_timer_;
  size_t max_batch_size_ = 100;
  std::chrono::milliseconds max_batch_latency_{1};

  explicit Impl(const std::string& socket_path) : socket_path_(socket_path) {}

  Impl(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> external_ioc)
      : socket_path_(socket_path),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel) : socket_path_(""), server_(std::move(channel)) {
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

  void flush_batches() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!data_batch_queue_.empty()) {
      auto handler = data_batch_handler_;
      auto batch = std::move(data_batch_queue_);
      data_batch_queue_.clear();
      if (handler) {
        lock.unlock();
        handler(batch);
        lock.lock();
      }
    }
    if (!message_batch_queue_.empty()) {
      auto handler = on_message_batch_;
      auto batch = std::move(message_batch_queue_);
      message_batch_queue_.clear();
      if (handler) {
        lock.unlock();
        handler(batch);
        lock.lock();
      }
    }
    if (batch_timer_) {
      batch_timer_->cancel();
    }
  }

  void schedule_batch_timer() {
    if (!batch_timer_) return;
    batch_timer_->expires_after(max_batch_latency_);
    batch_timer_->async_wait(
        [this, weak_alive = std::weak_ptr<bool>(alive_marker_)](const boost::system::error_code& ec) {
          if (ec) return;
          auto alive = weak_alive.lock();
          if (!alive) return;
          flush_batches();
        });
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

    if (!server_) {
      config::UdsServerConfig config;
      config.socket_path = socket_path_;
      config.idle_timeout_ms = idle_timeout_ms_.load();
      config.backpressure_threshold = backpressure_threshold_.load();
      config.backpressure_strategy = backpressure_strategy_.load();
      server_ = factory::ChannelFactory::create(config, external_ioc_);
      setup_internal_handlers();
    }

    lock.unlock();
    server_->start();
    if (use_external_context_.load() && manage_external_context_.load() && !external_thread_.joinable()) {
      if (external_ioc_ && external_ioc_->stopped()) {
        external_ioc_->restart();
      }
      work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          external_ioc_->get_executor());
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
        is_listening_.store(false);
        fulfill_all_locked(false);
        return;
      }
      if (batch_timer_) {
        batch_timer_->cancel();
        batch_timer_.reset();
      }
      if (server_) {
        server_->on_bytes(nullptr);
        server_->on_state(nullptr);
        lock.unlock();
        server_->stop();
        lock.lock();
      }
      if (use_external_context_.load() && manage_external_context_.load()) {
        if (work_guard_) work_guard_.reset();
        if (external_ioc_) external_ioc_->stop();
        should_join = true;
      }
      fulfill_all_locked(false);
      is_listening_.store(false);
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
    std::unique_lock<std::shared_mutex> lock(mutex_);
    server_.reset();
  }

  void setup_internal_handlers() {
    if (!server_) return;

    batch_timer_ = std::make_unique<boost::asio::steady_timer>(server_->get_executor());

    std::weak_ptr<bool> weak_alive = alive_marker_;
    auto transport_server = std::dynamic_pointer_cast<transport::UdsServer>(server_);
    if (transport_server) {
      transport_server->on_multi_connect([this, weak_alive](ClientId id, const std::string& info) {
        auto alive = weak_alive.lock();
        if (!alive) return;

        ConnectionHandler handler;
        {
          std::unique_lock<std::shared_mutex> lock(mutex_);
          if (framer_factory_) {
            auto framer = framer_factory_();
            if (framer) {
              framer->on_message([this, id](memory::ConstByteSpan msg) {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                if (on_message_batch_) {
                  message_batch_queue_.emplace_back(id, memory::SafeDataBuffer(msg));
                  if (message_batch_queue_.size() >= max_batch_size_) {
                    auto handler = on_message_batch_;
                    auto batch = std::move(message_batch_queue_);
                    message_batch_queue_.clear();
                    lock.unlock();
                    handler(batch);
                  } else if (message_batch_queue_.size() == 1) {
                    schedule_batch_timer();
                  }
                  return;
                }

                MessageHandler handler = on_message_;
                if (handler) {
                  lock.unlock();
                  handler(MessageContext(id, memory::SafeDataBuffer(msg)));
                  lock.lock();
                }
              });
              framers_[id] = std::move(framer);
            }
          }
          handler = client_connect_handler_;
        }
        if (handler) handler(ConnectionContext(id, info));
      });
      transport_server->on_multi_data([this, weak_alive](ClientId id, memory::ConstByteSpan data_span) {
        auto alive = weak_alive.lock();
        if (!alive) return;

        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (data_batch_handler_) {
          data_batch_queue_.emplace_back(id, memory::SafeDataBuffer(data_span));
          if (data_batch_queue_.size() >= max_batch_size_) {
            auto handler = data_batch_handler_;
            auto batch = std::move(data_batch_queue_);
            data_batch_queue_.clear();
            lock.unlock();
            handler(batch);
          } else if (data_batch_queue_.size() == 1) {
            schedule_batch_timer();
          }
          return;
        }

        MessageHandler handler = data_handler_;
        if (handler) {
          lock.unlock();
          handler(MessageContext(id, memory::SafeDataBuffer(data_span)));
          lock.lock();
        }

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
          std::unique_lock<std::shared_mutex> lock(mutex_);
          framers_.erase(id);
          handler = client_disconnect_handler_;
        }
        if (handler) handler(ConnectionContext(id));
      });
    }
    server_->on_state([this, weak_alive](base::LinkState state) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      if (state == base::LinkState::Listening) {
        is_listening_.store(true);
        std::unique_lock<std::shared_mutex> lock(mutex_);
        fulfill_all_locked(true);
      } else if (state == base::LinkState::Error || state == base::LinkState::Closed ||
                 state == base::LinkState::Idle) {
        ErrorHandler handler;
        is_listening_.store(false);
        {
          std::unique_lock<std::shared_mutex> lock(mutex_);
          fulfill_all_locked(false);
          if (state == base::LinkState::Error) {
            handler = error_handler_;
          }
        }
        if (handler) {
          handler(ErrorContext(ErrorCode::IoError, "Server error"));
        }
      }
    });
  }
};

UdsServer::UdsServer(const std::string& socket_path) : impl_(std::make_unique<Impl>(socket_path)) {}

UdsServer::UdsServer(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> ioc)
    : impl_(std::make_unique<Impl>(socket_path, std::move(ioc))) {}

UdsServer::UdsServer(std::shared_ptr<interface::Channel> channel) : impl_(std::make_unique<Impl>(std::move(channel))) {
  impl_->setup_internal_handlers();
}

UdsServer::~UdsServer() = default;

UdsServer::UdsServer(UdsServer&&) noexcept = default;
UdsServer& UdsServer::operator=(UdsServer&&) noexcept = default;

std::future<bool> UdsServer::start() { return impl_->start(); }

void UdsServer::stop() { impl_->stop(); }

bool UdsServer::listening() const { return impl_->is_listening_.load(); }

bool UdsServer::broadcast(std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  auto transport_server = std::dynamic_pointer_cast<transport::UdsServer>(impl_->server_);
  return transport_server ? transport_server->broadcast(data) : false;
}

bool UdsServer::send_to(ClientId client_id, std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  auto transport_server = std::dynamic_pointer_cast<transport::UdsServer>(impl_->server_);
  return transport_server ? transport_server->send_to_client(client_id, data) : false;
}

ServerInterface& UdsServer::on_connect(ConnectionHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->client_connect_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_disconnect(ConnectionHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->client_disconnect_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_data(MessageHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_data_batch(BatchMessageHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_batch_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_error(ErrorHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->error_handler_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::framer(FramerFactory factory) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->framer_factory_ = std::move(factory);
  return *this;
}

ServerInterface& UdsServer::on_message(MessageHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_message_ = std::move(handler);
  return *this;
}

ServerInterface& UdsServer::on_message_batch(BatchMessageHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_message_batch_ = std::move(handler);
  return *this;
}

size_t UdsServer::client_count() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  auto transport_server = std::dynamic_pointer_cast<transport::UdsServer>(impl_->server_);
  return transport_server ? transport_server->client_count() : 0;
}

std::vector<ClientId> UdsServer::connected_clients() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  auto transport_server = std::dynamic_pointer_cast<transport::UdsServer>(impl_->server_);
  return transport_server ? transport_server->connected_clients() : std::vector<ClientId>();
}

UdsServer& UdsServer::auto_start(bool manage) {
  impl_->auto_start_.store(manage);
  if (impl_->auto_start_.load() && !impl_->started_.load()) {
    start();
  }
  return *this;
}

UdsServer& UdsServer::idle_timeout(std::chrono::milliseconds timeout) {
  impl_->idle_timeout_ms_.store(static_cast<int>(timeout.count()));
  return *this;
}

UdsServer& UdsServer::max_clients(size_t max) {
  impl_->max_clients_.store(max);
  return *this;
}

UdsServer& UdsServer::unlimited_clients() {
  impl_->max_clients_.store(1000000);
  return *this;
}

UdsServer& UdsServer::backpressure_threshold(size_t threshold) {
  impl_->backpressure_threshold_.store(threshold);
  return *this;
}

UdsServer& UdsServer::backpressure_strategy(base::constants::BackpressureStrategy strategy) {
  impl_->backpressure_strategy_.store(strategy);
  return *this;
}

UdsServer& UdsServer::manage_external_context(bool manage) {
  impl_->manage_external_context_.store(manage);
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
