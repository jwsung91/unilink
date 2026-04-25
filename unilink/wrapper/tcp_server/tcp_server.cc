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
#include <boost/asio/steady_timer.hpp>
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
  std::atomic<bool> port_retry_enabled_{false};
  std::atomic<int> max_port_retries_{3};
  std::atomic<int> port_retry_interval_ms_{1000};
  std::atomic<int> idle_timeout_ms_{0};
  std::atomic<bool> client_limit_enabled_{false};
  std::atomic<size_t> max_clients_{0};

  ConnectionHandler on_client_connect_{nullptr};
  ConnectionHandler on_disconnect_{nullptr};
  MessageHandler on_data_{nullptr};
  BatchMessageHandler data_batch_handler_{nullptr};
  ErrorHandler on_error_{nullptr};
  FramerFactory framer_factory_{nullptr};
  MessageHandler on_message_{nullptr};
  BatchMessageHandler message_batch_handler_{nullptr};

  // Batching logic
  std::vector<MessageContext> data_batch_queue_;
  std::vector<MessageContext> message_batch_queue_;
  std::unique_ptr<boost::asio::steady_timer> batch_timer_;
  size_t max_batch_size_ = 100;
  std::chrono::milliseconds max_batch_latency_{1};

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
      auto handler = message_batch_handler_;
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

    if (!channel_) {
      config::TcpServerConfig config;
      config.port = port_;
      config.enable_port_retry = port_retry_enabled_.load();
      config.max_port_retries = max_port_retries_.load();
      config.port_retry_interval_ms = port_retry_interval_ms_.load();
      config.idle_timeout_ms = idle_timeout_ms_.load();

      channel_ = factory::ChannelFactory::create(config, external_ioc_);
      transport_cache_ = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
      setup_internal_handlers();

      if (client_limit_enabled_.load()) {
        auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
        if (transport_server) {
          if (max_clients_.load() == 0)
            transport_server->set_unlimited_clients();
          else
            transport_server->set_client_limit(max_clients_.load());
        }
      }
    }
    lock.unlock();
    channel_->start();
    if (use_external_context_.load() && manage_external_context_.load() && !external_thread_.joinable()) {
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
        is_listening_.store(false);
        fulfill_all_locked(false);
        return;
      }
      if (batch_timer_) {
        batch_timer_->cancel();
        batch_timer_.reset();
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
    channel_.reset();
  }

  void setup_internal_handlers() {
    if (!channel_) return;

    if (external_ioc_) {
      batch_timer_ = std::make_unique<boost::asio::steady_timer>(external_ioc_->get_executor());
    }

    std::weak_ptr<bool> weak_alive = alive_marker_;
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
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
                if (message_batch_handler_) {
                  message_batch_queue_.emplace_back(id, memory::SafeDataBuffer(msg));
                  if (message_batch_queue_.size() >= max_batch_size_) {
                    auto handler = message_batch_handler_;
                    auto batch = std::move(message_batch_queue_);
                    message_batch_queue_.clear();
                    lock.unlock();
                    handler(batch);
                  } else if (message_batch_queue_.size() == 1) {
                    schedule_batch_timer();
                  }
                  return;
                }

                MessageHandler on_message_handler = on_message_;
                if (on_message_handler) {
                  lock.unlock();
                  on_message_handler(MessageContext(id, memory::SafeDataBuffer(msg)));
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
        } else {
          MessageHandler handler = on_data_;
          if (handler) {
            lock.unlock();
            handler(MessageContext(id, memory::SafeDataBuffer(data_span)));
            lock.lock();
          }
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
          handler = on_disconnect_;
        }
        if (handler) handler(ConnectionContext(id));
      });
    }
    channel_->on_state([this, weak_alive](base::LinkState state) {
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
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  const auto& ts = impl_->transport_cache_;
  return ts ? ts->broadcast(data) : false;
}

bool TcpServer::send_to(ClientId client_id, std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  const auto& ts = impl_->transport_cache_;
  return ts ? ts->send_to_client(client_id, data) : false;
}

ServerInterface& TcpServer::on_connect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_client_connect_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_disconnect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_disconnect_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_data(MessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_data_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_data_batch(BatchMessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_batch_handler_ = std::move(h);
  return *this;
}
ServerInterface& TcpServer::on_error(ErrorHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_error_ = std::move(h);
  return *this;
}

ServerInterface& TcpServer::framer(FramerFactory factory) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->framer_factory_ = std::move(factory);
  return *this;
}

ServerInterface& TcpServer::on_message(MessageHandler handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->on_message_ = std::move(handler);
  return *this;
}

ServerInterface& TcpServer::on_message_batch(BatchMessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->message_batch_handler_ = std::move(h);
  return *this;
}

size_t TcpServer::client_count() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  const auto& ts = get_impl()->transport_cache_;
  return ts ? ts->client_count() : 0;
}

std::vector<ClientId> TcpServer::connected_clients() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  const auto& ts = get_impl()->transport_cache_;
  return ts ? ts->connected_clients() : std::vector<ClientId>();
}

TcpServer& TcpServer::auto_start(bool m) {
  impl_->auto_start_.store(m);
  if (impl_->auto_start_.load() && !impl_->started_.load()) start();
  return *this;
}

TcpServer& TcpServer::port_retry(bool e, int m, int i) {
  impl_->port_retry_enabled_.store(e);
  impl_->max_port_retries_.store(m);
  impl_->port_retry_interval_ms_.store(i);
  return *this;
}

TcpServer& TcpServer::idle_timeout(std::chrono::milliseconds timeout) {
  impl_->idle_timeout_ms_.store(static_cast<int>(timeout.count()));
  return *this;
}

TcpServer& TcpServer::max_clients(size_t max) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->max_clients_.store(max);
  impl_->client_limit_enabled_.store(true);
  if (impl_->transport_cache_) impl_->transport_cache_->set_client_limit(max);
  return *this;
}

TcpServer& TcpServer::unlimited_clients() {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->client_limit_enabled_.store(false);
  if (impl_->transport_cache_) impl_->transport_cache_->set_unlimited_clients();
  return *this;
}

TcpServer& TcpServer::manage_external_context(bool m) {
  impl_->manage_external_context_.store(m);
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
