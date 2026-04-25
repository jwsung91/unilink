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

#include "unilink/wrapper/udp/udp.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/udp/udp.hpp"

namespace unilink {
namespace wrapper {

struct UdpClient::Impl {
  mutable std::shared_mutex mutex_;
  config::UdpConfig cfg;
  std::shared_ptr<interface::Channel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard;

  // Event handlers (Context based)
  MessageHandler data_handler{nullptr};
  ConnectionHandler connect_handler{nullptr};
  ConnectionHandler disconnect_handler{nullptr};
  ErrorHandler error_handler{nullptr};
  MessageHandler message_handler{nullptr};

  std::unique_ptr<framer::IFramer> framer{nullptr};

  std::atomic<bool> auto_start_{false};
  std::vector<std::promise<bool>> pending_promises;
  std::atomic<bool> started_{false};
  std::shared_ptr<bool> alive_marker{std::make_shared<bool>(true)};

  explicit Impl(const config::UdpConfig& config) : cfg(config) {}
  Impl(const config::UdpConfig& config, std::shared_ptr<boost::asio::io_context> ioc)
      : cfg(config), external_ioc(std::move(ioc)), use_external_context(external_ioc != nullptr) {}
  explicit Impl(std::shared_ptr<interface::Channel> ch) : channel(std::move(ch)) {}

  ~Impl() {
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

  std::future<bool> start() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (channel && channel->is_connected()) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    std::promise<bool> p;
    auto future = p.get_future();
    pending_promises.emplace_back(std::move(p));

    if (started_.load()) {
      return future;
    }

    if (!channel) {
      channel = factory::ChannelFactory::create(cfg, external_ioc);
      setup_internal_handlers();
    }
    started_.store(true);

    lock.unlock();
    channel->start();
    if (use_external_context && manage_external_context && !external_thread.joinable()) {
      if (external_ioc->stopped()) external_ioc->restart();
      work_guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          boost::asio::make_work_guard(*external_ioc));
      external_thread = std::thread([this, ioc = external_ioc]() {
        try {
          ioc->run();
        } catch (...) {
        }
      });
    }
    return future;
  }

  void stop() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!started_.load()) {
      fulfill_all_locked(false);
      return;
    }
    started_.store(false);

    if (channel) {
      channel->on_bytes(nullptr);
      channel->on_state(nullptr);
      lock.unlock();
      channel->stop();
      lock.lock();
    }

    if (work_guard) {
      work_guard.reset();
    }

    if (use_external_context && manage_external_context && external_thread.joinable()) {
      if (external_ioc) {
        external_ioc->stop();
      }
      if (std::this_thread::get_id() != external_thread.get_id()) {
        lock.unlock();
        external_thread.join();
        lock.lock();
      } else {
        external_thread.detach();
      }
    }

    fulfill_all_locked(false);

    if (framer) {
      framer->reset();
    }
  }

  void setup_internal_handlers() {
    if (!channel) return;

    std::weak_ptr<bool> weak_alive = alive_marker;

    channel->on_bytes([this, weak_alive](memory::ConstByteSpan data) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      // 1. Raw data handler
      MessageHandler handler;
      {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        handler = data_handler;
      }
      if (handler) {
        std::string str_data = base::safe_convert::uint8_to_string(data.data(), data.size());
        handler(MessageContext(0, std::move(str_data)));
      }

      // 2. Framer integration
      std::shared_lock<std::shared_mutex> lock(mutex_);
      if (framer) {
        framer->push_bytes(data);
      }
    });

    channel->on_state([this, weak_alive](base::LinkState state) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      switch (state) {
        case base::LinkState::Connected:
        case base::LinkState::Listening: {
          ConnectionHandler handler;
          {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            fulfill_all_locked(true);
            handler = connect_handler;
          }
          if (handler) handler(ConnectionContext(0));
          break;
        }
        case base::LinkState::Closed:
        case base::LinkState::Error:
        case base::LinkState::Idle: {
          ConnectionHandler disconnect_handler_snapshot;
          ErrorHandler error_handler_snapshot;
          {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            fulfill_all_locked(false);
            if (state == base::LinkState::Error) {
              error_handler_snapshot = error_handler;
            } else {
              disconnect_handler_snapshot = disconnect_handler;
            }
          }
          if (disconnect_handler_snapshot) {
            disconnect_handler_snapshot(ConnectionContext(0));
          }
          if (error_handler_snapshot) {
            error_handler_snapshot(ErrorContext(ErrorCode::IoError, "Connection error"));
          }
          break;
        }
        default:
          break;
      }
    });
  }

  // Attach the stored message_handler to framer->on_message().
  // Must be called with mutex_ already held.
  void attach_framer_callback() {
    if (!framer) return;
    framer->on_message([this](memory::ConstByteSpan msg) {
      MessageHandler handler;
      {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        handler = message_handler;
      }
      if (handler) {
        std::string str_msg = base::safe_convert::uint8_to_string(msg.data(), msg.size());
        handler(MessageContext(0, std::move(str_msg)));
      }
    });
  }

  void set_framer(std::unique_ptr<framer::IFramer> f) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    framer = std::move(f);
    if (framer && message_handler) attach_framer_callback();
  }

  void on_message(MessageHandler handler) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    message_handler = std::move(handler);
    if (framer) attach_framer_callback();
  }
};

UdpClient::UdpClient(const config::UdpConfig& cfg) : impl_(std::make_unique<Impl>(cfg)) {}
UdpClient::UdpClient(const config::UdpConfig& cfg, std::shared_ptr<boost::asio::io_context> ioc)
    : impl_(std::make_unique<Impl>(cfg, ioc)) {}
UdpClient::UdpClient(std::shared_ptr<interface::Channel> ch) : impl_(std::make_unique<Impl>(ch)) {
  impl_->setup_internal_handlers();
}
UdpClient::~UdpClient() = default;

UdpClient::UdpClient(UdpClient&&) noexcept = default;
UdpClient& UdpClient::operator=(UdpClient&&) noexcept = default;

std::future<bool> UdpClient::start() { return impl_->start(); }
void UdpClient::stop() { impl_->stop(); }
bool UdpClient::send(std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  if (impl_->channel && impl_->channel->is_connected()) {
    auto binary_view = base::safe_convert::string_to_bytes(data);
    impl_->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    return true;
  }
  return false;
}
bool UdpClient::send_line(std::string_view line) { return send(std::string(line) + "\n"); }
bool UdpClient::connected() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  return impl_->channel && impl_->channel->is_connected();
}

ChannelInterface& UdpClient::on_data(MessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_handler = std::move(h);
  return *this;
}
ChannelInterface& UdpClient::on_connect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->connect_handler = std::move(h);
  return *this;
}
ChannelInterface& UdpClient::on_disconnect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->disconnect_handler = std::move(h);
  return *this;
}
ChannelInterface& UdpClient::on_error(ErrorHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->error_handler = std::move(h);
  return *this;
}

ChannelInterface& UdpClient::framer(std::unique_ptr<framer::IFramer> f) {
  impl_->set_framer(std::move(f));
  return *this;
}
ChannelInterface& UdpClient::on_message(MessageHandler h) {
  impl_->on_message(std::move(h));
  return *this;
}

ChannelInterface& UdpClient::auto_start(bool m) {
  impl_->auto_start_.store(m);
  if (impl_->auto_start_.load() && !impl_->started_.load()) start();
  return *this;
}

UdpClient& UdpClient::manage_external_context(bool manage) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->manage_external_context = manage;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
