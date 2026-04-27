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

#include "unilink/wrapper/udp/udp_server.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/udp/udp.hpp"

namespace unilink {
namespace wrapper {

namespace {
// std::hash<boost::asio::ip::udp::endpoint> is not available before Boost 1.74.
// Provide a portable hash by combining the raw address bytes and port.
struct UdpEndpointHash {
  std::size_t operator()(const boost::asio::ip::udp::endpoint& ep) const noexcept {
    std::size_t seed = 0;
    auto combine = [&](std::size_t v) { seed ^= v + 0x9e3779b9u + (seed << 6) + (seed >> 2); };
    if (ep.address().is_v4()) {
      for (auto byte : ep.address().to_v4().to_bytes()) {
        combine(std::hash<unsigned char>{}(byte));
      }
    } else {
      for (auto byte : ep.address().to_v6().to_bytes()) {
        combine(std::hash<unsigned char>{}(byte));
      }
    }
    combine(std::hash<unsigned short>{}(ep.port()));
    return seed;
  }
};
}  // namespace

struct UdpServer::Impl {
  config::UdpConfig cfg;
  std::shared_ptr<transport::UdpChannel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  std::atomic<bool> use_external_context{false};
  std::atomic<bool> manage_external_context{false};
  std::thread external_thread;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

  mutable std::shared_mutex mutex;
  std::vector<std::promise<bool>> pending_promises;
  std::atomic<bool> started{false};
  std::atomic<bool> is_listening{false};

  // Virtual Session Management
  struct SessionEntry {
    boost::asio::ip::udp::endpoint endpoint;
    std::shared_ptr<framer::IFramer> framer;
    std::chrono::steady_clock::time_point last_seen;
  };
  ClientId next_client_id{1};
  std::unordered_map<boost::asio::ip::udp::endpoint, ClientId, UdpEndpointHash> endpoint_to_id;
  std::unordered_map<ClientId, SessionEntry> sessions;
  std::chrono::milliseconds session_timeout{30000};  // Default 30s
  std::unique_ptr<boost::asio::steady_timer> reaper_timer;
  std::atomic<bool> auto_start{false};

  ConnectionHandler on_connect{nullptr};
  ConnectionHandler on_disconnect{nullptr};
  MessageHandler on_data{nullptr};
  BatchMessageHandler on_data_batch_{nullptr};
  ErrorHandler on_error{nullptr};
  std::function<void(size_t)> bp_handler{nullptr};
  FramerFactory framer_factory{nullptr};
  MessageHandler on_message{nullptr};
  BatchMessageHandler on_message_batch_{nullptr};

  std::shared_ptr<bool> is_alive{std::make_shared<bool>(true)};

  // Batching logic
  std::vector<MessageContext> data_batch_queue_;
  std::vector<MessageContext> message_batch_queue_;
  std::unique_ptr<boost::asio::steady_timer> batch_timer_;
  size_t max_batch_size_ = 100;
  std::chrono::milliseconds max_batch_latency_{1};

  explicit Impl(const config::UdpConfig& config) : cfg(config) {}
  Impl(const config::UdpConfig& config, std::shared_ptr<boost::asio::io_context> ioc)
      : cfg(config), external_ioc(std::move(ioc)), use_external_context(external_ioc != nullptr) {}
  explicit Impl(std::shared_ptr<interface::Channel> ch)
      : channel(std::dynamic_pointer_cast<transport::UdpChannel>(ch)) {
    setup_internal_handlers();
  }

  ~Impl() {
    *is_alive = false;
    try {
      stop();
    } catch (...) {
    }
  }

  void fulfill_all_locked(bool value) {
    for (auto& promise : pending_promises) {
      try {
        promise.set_value(value);
      } catch (...) {
      }
    }
    pending_promises.clear();
  }

  void flush_batches() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (!data_batch_queue_.empty()) {
      auto handler = on_data_batch_;
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
    batch_timer_->async_wait([this, alive = std::weak_ptr<bool>(is_alive)](const boost::system::error_code& ec) {
      auto lock = alive.lock();
      if (!lock || !(*lock)) return;

      if (!ec) {
        flush_batches();
      }
    });
  }

  void schedule_reaper() {
    if (!started.load() || !reaper_timer) return;

    // Run reaper at interval proportional to timeout (min 100ms, max 5s)
    auto interval =
        std::max(std::chrono::milliseconds(100), std::min(std::chrono::milliseconds(5000), session_timeout / 2));

    reaper_timer->expires_after(interval);
    reaper_timer->async_wait([this, alive = std::weak_ptr<bool>(is_alive)](const boost::system::error_code& ec) {
      auto lock = alive.lock();
      if (!lock || !(*lock)) return;

      if (!ec) {
        run_reaper();
        schedule_reaper();
      }
    });
  }

  void run_reaper() {
    std::vector<std::pair<ClientId, std::string>> to_remove_with_info;
    auto now = std::chrono::steady_clock::now();

    ConnectionHandler disconnect_handler;
    {
      std::unique_lock<std::shared_mutex> lock(mutex);
      for (auto it = sessions.begin(); it != sessions.end();) {
        if (now - it->second.last_seen > session_timeout) {
          std::string info =
              it->second.endpoint.address().to_string() + ":" + std::to_string(it->second.endpoint.port());
          endpoint_to_id.erase(it->second.endpoint);
          to_remove_with_info.push_back({it->first, info});
          it = sessions.erase(it);
        } else {
          ++it;
        }
      }
      disconnect_handler = on_disconnect;
    }

    // Call disconnect handlers outside the lock
    if (disconnect_handler) {
      for (auto const& [id, info] : to_remove_with_info) {
        disconnect_handler(ConnectionContext(id, info));
      }
    }
  }

  void setup_internal_handlers() {
    if (!channel) return;

    batch_timer_ = std::make_unique<boost::asio::steady_timer>(channel->get_executor());

    channel->on_bytes_from([this](memory::ConstByteSpan data, const boost::asio::ip::udp::endpoint& ep) {
      ClientId client_id = 0;
      bool is_new = false;
      ConnectionHandler connect_handler_copy{nullptr};

      {
        std::unique_lock<std::shared_mutex> lock(mutex);
        auto it = endpoint_to_id.find(ep);
        if (it == endpoint_to_id.end()) {
          client_id = next_client_id++;
          endpoint_to_id[ep] = client_id;
          SessionEntry entry;
          entry.endpoint = ep;
          entry.last_seen = std::chrono::steady_clock::now();
          is_new = true;

          // Create framer for new session
          if (framer_factory) {
            auto framer = framer_factory();
            if (framer) {
              framer->on_message([this, client_id](memory::ConstByteSpan msg) {
                std::unique_lock<std::shared_mutex> lock(mutex);
                if (on_message_batch_) {
                  message_batch_queue_.emplace_back(client_id, memory::SafeDataBuffer(msg));
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

                MessageHandler on_message_handler = on_message;
                if (on_message_handler) {
                  lock.unlock();
                  on_message_handler(MessageContext(client_id, memory::SafeDataBuffer(msg)));
                }
              });
              entry.framer = std::move(framer);
            }
          }
          sessions[client_id] = std::move(entry);
        } else {
          client_id = it->second;
          sessions[client_id].last_seen = std::chrono::steady_clock::now();
        }
        connect_handler_copy = on_connect;
      }

      if (is_new && connect_handler_copy) {
        connect_handler_copy(ConnectionContext(client_id, ep.address().to_string() + ":" + std::to_string(ep.port())));
      }

      {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (on_data_batch_) {
          data_batch_queue_.emplace_back(client_id, memory::SafeDataBuffer(data));
          if (data_batch_queue_.size() >= max_batch_size_) {
            auto handler = on_data_batch_;
            auto batch = std::move(data_batch_queue_);
            data_batch_queue_.clear();
            lock.unlock();
            handler(batch);
          } else if (data_batch_queue_.size() == 1) {
            schedule_batch_timer();
          }
        } else {
          MessageHandler data_handler_copy = on_data;
          if (data_handler_copy) {
            lock.unlock();
            data_handler_copy(MessageContext(client_id, memory::SafeDataBuffer(data)));
            lock.lock();
          }
        }
      }

      // Push to framer
      std::shared_ptr<framer::IFramer> target_framer;
      {
        std::shared_lock<std::shared_mutex> lock(mutex);
        auto it = sessions.find(client_id);
        if (it != sessions.end()) {
          target_framer = it->second.framer;
        }
      }
      if (target_framer) {
        target_framer->push_bytes(data);
      }
    });

    channel->on_backpressure([this](size_t queued) {
      std::shared_lock<std::shared_mutex> lock(mutex);
      if (bp_handler) bp_handler(queued);
    });

    channel->on_state([this](base::LinkState state) {
      ErrorHandler error_handler_copy{nullptr};
      if (state == base::LinkState::Listening || state == base::LinkState::Connected) {
        is_listening.store(true);
        std::unique_lock<std::shared_mutex> lock(mutex);
        fulfill_all_locked(true);
      } else if (state == base::LinkState::Error || state == base::LinkState::Closed ||
                 state == base::LinkState::Idle) {
        is_listening.store(false);
        std::unique_lock<std::shared_mutex> lock(mutex);
        fulfill_all_locked(false);
        if (state == base::LinkState::Error) {
          error_handler_copy = on_error;
        }
      }

      if (error_handler_copy) {
        error_handler_copy(ErrorContext(ErrorCode::IoError, "Server error"));
      }
    });
  }

  std::future<bool> start() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (is_listening.load()) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    std::promise<bool> p;
    auto fut = p.get_future();
    pending_promises.emplace_back(std::move(p));

    if (started.exchange(true)) {
      return fut;
    }

    if (!channel) {
      channel = std::dynamic_pointer_cast<transport::UdpChannel>(factory::ChannelFactory::create(cfg, external_ioc));
      setup_internal_handlers();
    }

    lock.unlock();
    channel->start();

    lock.lock();
    if (use_external_context.load() && manage_external_context.load() && !external_thread.joinable()) {
      if (external_ioc->stopped()) external_ioc->restart();
      work_guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          external_ioc->get_executor());
      external_thread = std::thread([ioc = external_ioc]() {
        try {
          ioc->run();
        } catch (...) {
        }
      });
    }

    if (channel) {
      reaper_timer = std::make_unique<boost::asio::steady_timer>(channel->get_executor());
      schedule_reaper();
    }

    return fut;
  }

  void stop() {
    bool should_join = false;
    {
      std::unique_lock<std::shared_mutex> lock(mutex);
      if (!started.exchange(false)) {
        is_listening.store(false);
        fulfill_all_locked(false);
        return;
      }

      if (reaper_timer) {
        reaper_timer->cancel();
        reaper_timer.reset();
      }

      if (batch_timer_) {
        batch_timer_->cancel();
        batch_timer_.reset();
      }

      if (channel) {
        channel->on_bytes_from(nullptr);
        channel->on_state(nullptr);
        lock.unlock();
        channel->stop();
        lock.lock();
      }

      if (use_external_context.load() && manage_external_context.load()) {
        if (external_ioc) external_ioc->stop();
        should_join = true;
      }

      is_listening.store(false);
      endpoint_to_id.clear();
      sessions.clear();
      next_client_id = 1;
      fulfill_all_locked(false);
    }

    if (should_join && external_thread.joinable()) {
      try {
        if (std::this_thread::get_id() != external_thread.get_id()) {
          external_thread.join();
        } else {
          external_thread.detach();
        }
      } catch (...) {
      }
    }
    std::unique_lock<std::shared_mutex> lock(mutex);
    channel.reset();
  }
};

UdpServer::UdpServer(uint16_t port) {
  config::UdpConfig cfg;
  cfg.local_port = port;
  impl_ = std::make_unique<Impl>(cfg);
}

UdpServer::UdpServer(const config::UdpConfig& cfg) : impl_(std::make_unique<Impl>(cfg)) {}

UdpServer::UdpServer(const config::UdpConfig& cfg, std::shared_ptr<boost::asio::io_context> ioc)
    : impl_(std::make_unique<Impl>(cfg, ioc)) {}

UdpServer::UdpServer(std::shared_ptr<interface::Channel> ch) : impl_(std::make_unique<Impl>(std::move(ch))) {}

UdpServer::~UdpServer() = default;

UdpServer::UdpServer(UdpServer&&) noexcept = default;
UdpServer& UdpServer::operator=(UdpServer&&) noexcept = default;

std::future<bool> UdpServer::start() { return impl_->start(); }
void UdpServer::stop() { impl_->stop(); }
bool UdpServer::listening() const { return impl_->is_listening.load(); }

bool UdpServer::broadcast(std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex);
  if (!impl_->channel) return false;

  auto bytes = base::safe_convert::string_to_bytes(data);
  for (const auto& [id, entry] : impl_->sessions) {
    impl_->channel->async_write_to(memory::ConstByteSpan(bytes.first, bytes.second), entry.endpoint);
  }
  return true;
}

bool UdpServer::send_to(ClientId client_id, std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex);
  if (!impl_->channel) return false;

  auto it = impl_->sessions.find(client_id);
  if (it == impl_->sessions.end()) return false;

  auto bytes = base::safe_convert::string_to_bytes(data);
  impl_->channel->async_write_to(memory::ConstByteSpan(bytes.first, bytes.second), it->second.endpoint);
  return true;
}

ServerInterface& UdpServer::on_connect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_connect = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_disconnect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_disconnect = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_data(MessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_data = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_data_batch(BatchMessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_data_batch_ = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_error(ErrorHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_error = std::move(h);
  return *this;
}

ServerInterface& UdpServer::framer(FramerFactory factory) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->framer_factory = std::move(factory);
  return *this;
}

ServerInterface& UdpServer::on_message(MessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_message = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_message_batch(BatchMessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->on_message_batch_ = std::move(h);
  return *this;
}

size_t UdpServer::client_count() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex);
  return impl_->endpoint_to_id.size();
}

std::vector<ClientId> UdpServer::connected_clients() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex);
  std::vector<ClientId> ids;
  ids.reserve(impl_->sessions.size());
  for (const auto& [id, entry] : impl_->sessions) {
    ids.push_back(id);
  }
  return ids;
}

UdpServer& UdpServer::auto_start(bool m) {
  impl_->auto_start.store(m);
  if (impl_->auto_start.load() && !impl_->started.load()) {
    start();
  }
  return *this;
}

UdpServer& UdpServer::session_timeout(std::chrono::milliseconds timeout) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->session_timeout = timeout;
  return *this;
}

UdpServer& UdpServer::on_backpressure(std::function<void(size_t)> handler) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->bp_handler = std::move(handler);
  return *this;
}

UdpServer& UdpServer::backpressure_threshold(size_t threshold) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->cfg.backpressure_threshold = threshold;
  return *this;
}

UdpServer& UdpServer::backpressure_strategy(base::constants::BackpressureStrategy strategy) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex);
  impl_->cfg.backpressure_strategy = strategy;
  if (impl_->channel) {
    impl_->channel->set_backpressure_strategy(strategy);
  }
  return *this;
}

UdpServer& UdpServer::manage_external_context(bool m) {
  impl_->manage_external_context.store(m);
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
