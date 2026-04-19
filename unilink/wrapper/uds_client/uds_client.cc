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

#include "unilink/wrapper/uds_client/uds_client.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/diagnostics/error_mapping.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/uds/uds_client.hpp"

namespace unilink {
namespace wrapper {

struct UdsClient::Impl {
  mutable std::mutex mutex_;
  std::string socket_path_;
  std::shared_ptr<interface::Channel> channel_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  std::atomic<bool> started_{false};
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  MessageHandler data_handler_{nullptr};
  ConnectionHandler connect_handler_{nullptr};
  ConnectionHandler disconnect_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};
  MessageHandler message_handler_{nullptr};

  std::unique_ptr<framer::IFramer> framer_{nullptr};

  bool auto_manage_ = false;
  std::chrono::milliseconds retry_interval_{3000};
  int max_retries_ = -1;
  std::chrono::milliseconds connection_timeout_{5000};

  explicit Impl(const std::string& socket_path) : socket_path_(socket_path), started_(false) {}

  Impl(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> external_ioc)
      : socket_path_(socket_path),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr),
        manage_external_context_(false),
        started_(false) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel)
      : socket_path_(""), channel_(std::move(channel)), started_(false) {
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
    std::unique_lock<std::mutex> lock(mutex_);
    if (channel_ && channel_->is_connected()) {
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

    if (!channel_) {
      config::UdsClientConfig cfg;
      cfg.socket_path = socket_path_;
      cfg.retry_interval_ms = static_cast<unsigned>(retry_interval_.count());
      cfg.max_retries = max_retries_;
      cfg.connection_timeout_ms = static_cast<unsigned>(connection_timeout_.count());

      if (use_external_context_) {
        channel_ = factory::ChannelFactory::create(cfg, external_ioc_);
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
        channel_ = factory::ChannelFactory::create(cfg);
      }
      setup_internal_handlers();
    }

    lock.unlock();  // UNLOCK BEFORE START
    channel_->start();
    lock.lock();

    return f;
  }

  void stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_.exchange(false)) {
      return;
    }

    // RELEASE LOCK before calling channel_->stop() because it might trigger
    // callbacks that try to acquire this same lock (e.g., on_state -> fulfill_all)
    if (channel_) {
      channel_->on_bytes(nullptr);
      channel_->on_state(nullptr);
      lock.unlock();
      channel_->stop();
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

    fulfill_all_locked(false);
    channel_.reset();
    if (framer_) {
      framer_->reset();
    }
  }

  void setup_internal_handlers() {
    if (!channel_) return;

    std::weak_ptr<bool> weak_alive = alive_marker_;

    channel_->on_state([this, weak_alive](base::LinkState state) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      if (state == base::LinkState::Connected) {
        ConnectionHandler handler;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          fulfill_all_locked(true);
          handler = connect_handler_;
        }
        if (handler) {
          handler(ConnectionContext(0));
        }
      } else if (state == base::LinkState::Error) {
        ErrorHandler handler;
        std::shared_ptr<interface::Channel> channel_snapshot;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          fulfill_all_locked(false);
          handler = error_handler_;
          channel_snapshot = channel_;
        }
        if (handler) {
          bool handled = false;
          if (auto transport = std::dynamic_pointer_cast<transport::UdsClient>(channel_snapshot)) {
            if (auto info = transport->last_error_info()) {
              handler(diagnostics::to_error_context(*info));
              handled = true;
            }
          }
          if (!handled) {
            handler(ErrorContext(ErrorCode::IoError, "Connection error"));
          }
        }
      } else if (state == base::LinkState::Closed || state == base::LinkState::Idle) {
        ConnectionHandler handler;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          fulfill_all_locked(false);
          handler = disconnect_handler_;
        }
        if (handler) {
          handler(ConnectionContext(0));
        }
      }
    });

    channel_->on_bytes([this, weak_alive](memory::ConstByteSpan data) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      // 1. Raw data handler
      MessageHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = data_handler_;
      }
      if (handler) {
        handler(MessageContext(0, base::safe_convert::uint8_to_string(data.data(), data.size())));
      }

      // 2. Framer integration
      std::lock_guard<std::mutex> lock(mutex_);
      if (framer_) {
        framer_->push_bytes(data);
      }
    });
  }

  // Attach the stored message_handler_ to framer_->on_message().
  // Must be called with mutex_ already held.
  void attach_framer_callback() {
    if (!framer_) return;
    framer_->on_message([this](memory::ConstByteSpan msg) {
      MessageHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = message_handler_;
      }
      if (handler) {
        handler(MessageContext(0, base::safe_convert::uint8_to_string(msg.data(), msg.size())));
      }
    });
  }

  void set_framer(std::unique_ptr<framer::IFramer> framer) {
    std::lock_guard<std::mutex> lock(mutex_);
    framer_ = std::move(framer);
    if (framer_ && message_handler_) attach_framer_callback();
  }

  void on_message(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_handler_ = std::move(handler);
    if (framer_) attach_framer_callback();
  }
};

UdsClient::UdsClient(const std::string& socket_path) : impl_(std::make_unique<Impl>(socket_path)) {}

UdsClient::UdsClient(const std::string& socket_path, std::shared_ptr<boost::asio::io_context> external_ioc)
    : impl_(std::make_unique<Impl>(socket_path, std::move(external_ioc))) {}

UdsClient::UdsClient(std::shared_ptr<interface::Channel> channel) : impl_(std::make_unique<Impl>(std::move(channel))) {}

UdsClient::~UdsClient() = default;

UdsClient::UdsClient(UdsClient&&) noexcept = default;
UdsClient& UdsClient::operator=(UdsClient&&) noexcept = default;

std::future<bool> UdsClient::start() { return impl_->start(); }

void UdsClient::stop() { impl_->stop(); }

bool UdsClient::send(std::string_view data) {
  if (impl_->channel_) {
    impl_->channel_->async_write_copy(
        memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    return true;
  }
  return false;
}

bool UdsClient::send_line(std::string_view line) {
  std::string data(line);
  data += "\n";
  return send(data);
}

bool UdsClient::connected() const { return impl_->channel_ && impl_->channel_->is_connected(); }

ChannelInterface& UdsClient::on_data(MessageHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->data_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& UdsClient::on_connect(ConnectionHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->connect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& UdsClient::on_disconnect(ConnectionHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->disconnect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& UdsClient::on_error(ErrorHandler handler) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->error_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& UdsClient::framer(std::unique_ptr<framer::IFramer> f) {
  impl_->set_framer(std::move(f));
  return *this;
}

ChannelInterface& UdsClient::on_message(MessageHandler h) {
  impl_->on_message(std::move(h));
  return *this;
}

ChannelInterface& UdsClient::auto_manage(bool manage) {
  impl_->auto_manage_ = manage;
  if (impl_->auto_manage_ && !impl_->started_.load()) {
    start();
  }
  return *this;
}

UdsClient& UdsClient::retry_interval(std::chrono::milliseconds interval) {
  impl_->retry_interval_ = interval;
  if (impl_->channel_) {
    auto transport_client = std::dynamic_pointer_cast<transport::UdsClient>(impl_->channel_);
    if (transport_client) transport_client->set_retry_interval(static_cast<unsigned int>(interval.count()));
  }
  return *this;
}

UdsClient& UdsClient::max_retries(int max_retries) {
  impl_->max_retries_ = max_retries;
  return *this;
}

UdsClient& UdsClient::connection_timeout(std::chrono::milliseconds timeout) {
  impl_->connection_timeout_ = timeout;
  return *this;
}

UdsClient& UdsClient::manage_external_context(bool manage) {
  impl_->manage_external_context_ = manage;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
