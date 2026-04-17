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

#include "unilink/wrapper/serial/serial.hpp"

#include <algorithm>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <cctype>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/serial/serial.hpp"

namespace unilink {
namespace wrapper {

namespace {
std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}
}  // namespace

struct Serial::Impl {
  mutable std::mutex mutex_;
  std::string device;
  uint32_t baud_rate;
  std::shared_ptr<interface::Channel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  std::atomic<bool> started_{false};
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  // Event handlers (Context based)
  MessageHandler data_handler{nullptr};
  ConnectionHandler connect_handler{nullptr};
  ConnectionHandler disconnect_handler{nullptr};
  ErrorHandler error_handler{nullptr};
  MessageHandler message_handler{nullptr};

  std::unique_ptr<framer::IFramer> framer{nullptr};

  // Configuration
  bool auto_manage = false;
  int data_bits = 8;
  int stop_bits = 1;
  std::string parity = "none";
  std::string flow_control = "none";
  std::chrono::milliseconds retry_interval{3000};

  Impl(const std::string& dev, uint32_t baud) : device(dev), baud_rate(baud) {}
  Impl(const std::string& dev, uint32_t baud, std::shared_ptr<boost::asio::io_context> ioc)
      : device(dev), baud_rate(baud), external_ioc(std::move(ioc)), use_external_context(external_ioc != nullptr) {}
  explicit Impl(std::shared_ptr<interface::Channel> ch) : channel(std::move(ch)) {}

  ~Impl() {
    try {
      stop();
    } catch (...) {
    }
  }

  void fulfill_all_locked(bool value) {
    for (auto& promise : pending_promises_) {
      try {
        promise.set_value(value);
      } catch (...) {
      }
    }
    pending_promises_.clear();
  }

  std::future<bool> start() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (channel && channel->is_connected()) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    std::promise<bool> p;
    auto future = p.get_future();
    pending_promises_.emplace_back(std::move(p));

    if (started_.exchange(true)) {
      return future;
    }

    if (!channel) {
      channel = factory::ChannelFactory::create(build_config(), external_ioc);
      setup_internal_handlers();
    }

    lock.unlock();
    channel->start();
    if (use_external_context && manage_external_context && !external_thread.joinable()) {
      if (external_ioc && external_ioc->stopped()) {
        external_ioc->restart();
      }
      work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          boost::asio::make_work_guard(*external_ioc));
      external_thread = std::thread([ioc = external_ioc]() {
        try {
          ioc->run();
        } catch (...) {
        }
      });
    }

    return future;
  }

  void stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_.exchange(false)) {
      fulfill_all_locked(false);
      return;
    }

    if (channel) {
      channel->on_bytes(nullptr);
      channel->on_state(nullptr);
      lock.unlock();
      channel->stop();
      lock.lock();
    }

    if (work_guard_) {
      work_guard_.reset();
    }

    if (use_external_context && manage_external_context && external_thread.joinable()) {
      if (external_ioc) external_ioc->stop();
      if (std::this_thread::get_id() != external_thread.get_id()) {
        lock.unlock();
        external_thread.join();
        lock.lock();
      } else {
        external_thread.detach();
      }
    }

    fulfill_all_locked(false);

    if (framer) framer->reset();
  }

  void setup_internal_handlers() {
    if (!channel) return;

    std::weak_ptr<bool> weak_alive = alive_marker_;

    channel->on_bytes([this, weak_alive](memory::ConstByteSpan data) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      // 1. Raw data handler
      MessageHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = data_handler;
      }
      if (handler) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        handler(MessageContext(0, str_data));
      }

      // 2. Framer integration
      std::lock_guard<std::mutex> lock(mutex_);
      if (framer) {
        framer->push_bytes(data);
      }
    });

    channel->on_state([this, weak_alive](base::LinkState state) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      switch (state) {
        case base::LinkState::Connected: {
          ConnectionHandler handler;
          {
            std::lock_guard<std::mutex> lock(mutex_);
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
            std::lock_guard<std::mutex> lock(mutex_);
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

  // Attach the stored message_handler to framer->set_on_message().
  // Must be called with mutex_ already held.
  void attach_framer_callback() {
    if (!framer) return;
    framer->set_on_message([this](memory::ConstByteSpan msg) {
      MessageHandler handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = message_handler;
      }
      if (handler) {
        handler(MessageContext(0, common::safe_convert::uint8_to_string(msg.data(), msg.size())));
      }
    });
  }

  void set_framer(std::unique_ptr<framer::IFramer> f) {
    std::lock_guard<std::mutex> lock(mutex_);
    framer = std::move(f);
    if (framer && message_handler) attach_framer_callback();
  }

  void on_message(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_handler = std::move(handler);
    if (framer) attach_framer_callback();
  }

  config::SerialConfig build_config() const {
    config::SerialConfig config;
    config.device = device;
    config.baud_rate = baud_rate;
    config.char_size = static_cast<unsigned int>(data_bits);
    config.stop_bits = static_cast<unsigned int>(stop_bits);
    std::string p = to_lower(parity);
    if (p == "even")
      config.parity = config::SerialConfig::Parity::Even;
    else if (p == "odd")
      config.parity = config::SerialConfig::Parity::Odd;
    else
      config.parity = config::SerialConfig::Parity::None;

    std::string f = to_lower(flow_control);
    if (f == "software")
      config.flow = config::SerialConfig::Flow::Software;
    else if (f == "hardware")
      config.flow = config::SerialConfig::Flow::Hardware;
    else
      config.flow = config::SerialConfig::Flow::None;

    config.retry_interval_ms = static_cast<unsigned int>(retry_interval.count());
    return config;
  }
};

Serial::Serial(const std::string& d, uint32_t b) : impl_(std::make_unique<Impl>(d, b)) {}
Serial::Serial(const std::string& d, uint32_t b, std::shared_ptr<boost::asio::io_context> i)
    : impl_(std::make_unique<Impl>(d, b, i)) {}
Serial::Serial(std::shared_ptr<interface::Channel> ch) : impl_(std::make_unique<Impl>(ch)) {
  impl_->setup_internal_handlers();
}
Serial::~Serial() = default;

Serial::Serial(Serial&&) noexcept = default;
Serial& Serial::operator=(Serial&&) noexcept = default;

std::future<bool> Serial::start() { return impl_->start(); }
void Serial::stop() { impl_->stop(); }
void Serial::send(std::string_view data) {
  if (is_connected() && get_impl()->channel) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    get_impl()->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}
void Serial::send_line(std::string_view line) { send(std::string(line) + "\n"); }
bool Serial::is_connected() const { return get_impl()->channel && get_impl()->channel->is_connected(); }

ChannelInterface& Serial::on_data(MessageHandler h) {
  impl_->data_handler = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_connect(ConnectionHandler h) {
  impl_->connect_handler = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_disconnect(ConnectionHandler h) {
  impl_->disconnect_handler = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_error(ErrorHandler h) {
  impl_->error_handler = std::move(h);
  return *this;
}

ChannelInterface& Serial::framer(std::unique_ptr<framer::IFramer> f) {
  impl_->set_framer(std::move(f));
  return *this;
}
ChannelInterface& Serial::on_message(MessageHandler h) {
  impl_->on_message(std::move(h));
  return *this;
}

ChannelInterface& Serial::auto_manage(bool m) {
  impl_->auto_manage = m;
  if (impl_->auto_manage && !impl_->started_.load()) start();
  return *this;
}

Serial& Serial::baud_rate(uint32_t b) {
  impl_->baud_rate = b;
  return *this;
}
Serial& Serial::data_bits(int d) {
  impl_->data_bits = d;
  return *this;
}
Serial& Serial::stop_bits(int s) {
  impl_->stop_bits = s;
  return *this;
}
Serial& Serial::parity(const std::string& p) {
  impl_->parity = p;
  return *this;
}
Serial& Serial::flow_control(const std::string& f) {
  impl_->flow_control = f;
  return *this;
}
Serial& Serial::retry_interval(std::chrono::milliseconds i) {
  impl_->retry_interval = i;
  if (impl_->channel) {
    auto ts = std::dynamic_pointer_cast<transport::Serial>(impl_->channel);
    if (ts) ts->set_retry_interval(static_cast<unsigned int>(i.count()));
  }
  return *this;
}

config::SerialConfig Serial::build_config() const { return get_impl()->build_config(); }

Serial& Serial::manage_external_context(bool m) {
  impl_->manage_external_context = m;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
