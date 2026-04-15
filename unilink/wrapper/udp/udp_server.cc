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
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/udp/udp.hpp"

namespace unilink {
namespace wrapper {

struct UdpServer::Impl {
  config::UdpConfig cfg;
  std::shared_ptr<transport::UdpChannel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

  mutable std::mutex mutex;
  std::vector<std::promise<bool>> pending_promises;
  std::atomic<bool> started{false};
  std::atomic<bool> is_listening{false};

  // Virtual Session Management
  size_t next_client_id{1};
  std::map<boost::asio::ip::udp::endpoint, size_t> endpoint_to_id;
  std::map<size_t, boost::asio::ip::udp::endpoint> id_to_endpoint;
  std::map<size_t, std::shared_ptr<framer::IFramer>> client_framers;
  std::map<size_t, std::chrono::steady_clock::time_point> session_activity;
  std::chrono::milliseconds session_timeout{30000};  // Default 30s
  std::unique_ptr<boost::asio::steady_timer> reaper_timer;
  bool auto_manage{false};

  ConnectionHandler on_connect{nullptr};
  ConnectionHandler on_disconnect{nullptr};
  MessageHandler on_data{nullptr};
  ErrorHandler on_error{nullptr};
  FramerFactory framer_factory{nullptr};
  MessageHandler on_message{nullptr};

  std::shared_ptr<bool> is_alive{std::make_shared<bool>(true)};

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
    std::vector<std::pair<size_t, std::string>> to_remove_with_info;
    auto now = std::chrono::steady_clock::now();

    {
      std::lock_guard<std::mutex> lock(mutex);
      for (auto const& [id, last_seen] : session_activity) {
        if (now - last_seen > session_timeout) {
          auto it_ep = id_to_endpoint.find(id);
          std::string info = "timeout";
          if (it_ep != id_to_endpoint.end()) {
            info = it_ep->second.address().to_string() + ":" + std::to_string(it_ep->second.port());
            endpoint_to_id.erase(it_ep->second);
            id_to_endpoint.erase(it_ep);
          }
          client_framers.erase(id);
          to_remove_with_info.push_back({id, info});
        }
      }

      for (auto const& [id, info] : to_remove_with_info) {
        session_activity.erase(id);
      }
    }

    // Call disconnect handlers outside the lock
    if (on_disconnect) {
      for (auto const& [id, info] : to_remove_with_info) {
        on_disconnect(ConnectionContext(id, info));
      }
    }
  }

  void setup_internal_handlers() {
    if (!channel) return;

    channel->on_bytes_from([this](memory::ConstByteSpan data, const boost::asio::ip::udp::endpoint& ep) {
      size_t client_id = 0;
      bool is_new = false;
      ConnectionHandler connect_handler_copy{nullptr};
      MessageHandler data_handler_copy{nullptr};

      {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = endpoint_to_id.find(ep);
        if (it == endpoint_to_id.end()) {
          client_id = next_client_id++;
          endpoint_to_id[ep] = client_id;
          id_to_endpoint[client_id] = ep;
          is_new = true;

          // Create framer for new session
          if (framer_factory) {
            auto framer = framer_factory();
            if (framer) {
              framer->set_on_message([this, client_id](memory::ConstByteSpan msg) {
                MessageHandler on_message_handler;
                {
                  std::lock_guard<std::mutex> lock(mutex);
                  on_message_handler = on_message;
                }
                if (on_message_handler) {
                  std::string str_msg = common::safe_convert::uint8_to_string(msg.data(), msg.size());
                  on_message_handler(MessageContext(client_id, str_msg));
                }
              });
              client_framers[client_id] = std::shared_ptr<framer::IFramer>(std::move(framer));
            }
          }
        } else {
          client_id = it->second;
        }
        session_activity[client_id] = std::chrono::steady_clock::now();
        connect_handler_copy = on_connect;
        data_handler_copy = on_data;
      }

      if (is_new && connect_handler_copy) {
        connect_handler_copy(ConnectionContext(client_id, ep.address().to_string() + ":" + std::to_string(ep.port())));
      }

      if (data_handler_copy) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler_copy(MessageContext(client_id, str_data));
      }

      // Push to framer
      std::shared_ptr<framer::IFramer> target_framer;
      {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = client_framers.find(client_id);
        if (it != client_framers.end()) {
          target_framer = it->second;
        }
      }
      if (target_framer) {
        target_framer->push_bytes(data);
      }
    });

    channel->on_state([this](base::LinkState state) {
      ErrorHandler error_handler_copy{nullptr};
      if (state == base::LinkState::Listening || state == base::LinkState::Connected) {
        std::lock_guard<std::mutex> lock(mutex);
        is_listening = true;
        fulfill_all_locked(true);
      } else if (state == base::LinkState::Error || state == base::LinkState::Closed ||
                 state == base::LinkState::Idle) {
        std::lock_guard<std::mutex> lock(mutex);
        is_listening = false;
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
    std::unique_lock<std::mutex> lock(mutex);
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
    if (use_external_context && manage_external_context && !external_thread.joinable()) {
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
      std::unique_lock<std::mutex> lock(mutex);
      if (!started.exchange(false)) {
        is_listening = false;
        fulfill_all_locked(false);
        return;
      }

      if (reaper_timer) {
        reaper_timer->cancel();
        reaper_timer.reset();
      }

      if (channel) {
        channel->on_bytes_from(nullptr);
        channel->on_state(nullptr);
        lock.unlock();
        channel->stop();
        lock.lock();
      }

      if (use_external_context && manage_external_context && external_thread.joinable()) {
        if (external_ioc) external_ioc->stop();
        should_join = true;
      }

      is_listening = false;
      endpoint_to_id.clear();
      id_to_endpoint.clear();
      client_framers.clear();
      session_activity.clear();
      next_client_id = 1;
      fulfill_all_locked(false);
    }

    if (should_join) {
      try {
        if (std::this_thread::get_id() != external_thread.get_id()) {
          external_thread.join();
        } else {
          external_thread.detach();
        }
      } catch (...) {
      }
    }
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
bool UdpServer::is_listening() const { return impl_->is_listening.load(); }

bool UdpServer::broadcast(std::string_view data) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->channel) return false;

  auto bytes = common::safe_convert::string_to_bytes(data);
  for (const auto& pair : impl_->id_to_endpoint) {
    impl_->channel->async_write_to(memory::ConstByteSpan(bytes.first, bytes.second), pair.second);
  }
  return true;
}

bool UdpServer::send_to(size_t client_id, std::string_view data) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (!impl_->channel) return false;

  auto it = impl_->id_to_endpoint.find(client_id);
  if (it == impl_->id_to_endpoint.end()) return false;

  auto bytes = common::safe_convert::string_to_bytes(data);
  impl_->channel->async_write_to(memory::ConstByteSpan(bytes.first, bytes.second), it->second);
  return true;
}

ServerInterface& UdpServer::on_client_connect(ConnectionHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_connect = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_client_disconnect(ConnectionHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_disconnect = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_data(MessageHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_data = std::move(h);
  return *this;
}

ServerInterface& UdpServer::on_error(ErrorHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_error = std::move(h);
  return *this;
}

ServerInterface& UdpServer::framer_factory(FramerFactory factory) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->framer_factory = std::move(factory);
  return *this;
}

ServerInterface& UdpServer::on_message(MessageHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->on_message = std::move(h);
  return *this;
}

size_t UdpServer::client_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->endpoint_to_id.size();
}

std::vector<size_t> UdpServer::connected_clients() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  std::vector<size_t> ids;
  for (const auto& pair : impl_->id_to_endpoint) {
    ids.push_back(pair.first);
  }
  return ids;
}

UdpServer& UdpServer::auto_manage(bool m) {
  impl_->auto_manage = m;
  if (impl_->auto_manage && !impl_->started.load()) {
    start();
  }
  return *this;
}

UdpServer& UdpServer::session_timeout(std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->session_timeout = timeout;
  return *this;
}

UdpServer& UdpServer::set_manage_external_context(bool m) {
  impl_->manage_external_context = m;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
