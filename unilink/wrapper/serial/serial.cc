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
#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <cctype>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/base/constants.hpp"
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
  mutable std::shared_mutex mutex_;
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
  BatchMessageHandler data_batch_handler_{nullptr};
  ConnectionHandler connect_handler{nullptr};
  ConnectionHandler disconnect_handler{nullptr};
  ErrorHandler error_handler{nullptr};
  MessageHandler message_handler{nullptr};
  BatchMessageHandler message_batch_handler_{nullptr};

  std::unique_ptr<framer::IFramer> framer{nullptr};

  // Batching logic
  std::vector<MessageContext> data_batch_queue_;
  std::vector<MessageContext> message_batch_queue_;
  std::unique_ptr<boost::asio::steady_timer> batch_timer_;
  size_t max_batch_size_ = 100;
  std::chrono::milliseconds max_batch_latency_{1};

  // Configuration
  std::atomic<bool> auto_start_ = false;
  int data_bits = 8;
  int stop_bits = 1;
  std::string parity = "none";
  std::string flow_control = "none";
  std::chrono::milliseconds retry_interval{base::constants::DEFAULT_RETRY_INTERVAL_MS};
  size_t backpressure_threshold = base::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  base::constants::BackpressureStrategy backpressure_strategy = base::constants::BackpressureStrategy::KeepAll;

  Impl(const std::string& dev, uint32_t baud) : device(dev), baud_rate(baud) {}
  Impl(const std::string& dev, uint32_t baud, std::shared_ptr<boost::asio::io_context> ioc)
      : device(dev), baud_rate(baud), external_ioc(std::move(ioc)), use_external_context(external_ioc != nullptr) {}
  explicit Impl(std::shared_ptr<interface::Channel> ch) : channel(std::move(ch)) { setup_internal_handlers(); }

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
    if (channel && channel->is_connected()) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    std::promise<bool> p;
    auto future = p.get_future();
    pending_promises_.emplace_back(std::move(p));

    if (started_.load()) {
      return future;
    }

    if (!channel) {
      channel = factory::ChannelFactory::create(build_config_locked(), external_ioc);
      setup_internal_handlers();
    }
    started_.store(true);

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
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (!started_.load()) {
      fulfill_all_locked(false);
      return;
    }
    started_.store(false);

    if (batch_timer_) {
      batch_timer_->cancel();
      batch_timer_.reset();
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

    batch_timer_ = std::make_unique<boost::asio::steady_timer>(channel->get_executor());

    std::weak_ptr<bool> weak_alive = alive_marker_;

    channel->on_bytes([this, weak_alive](memory::ConstByteSpan data) {
      auto alive = weak_alive.lock();
      if (!alive) return;

      std::unique_lock<std::shared_mutex> lock(mutex_);
      if (data_batch_handler_) {
        data_batch_queue_.emplace_back(0, memory::SafeDataBuffer(data));
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

      MessageHandler handler = data_handler;
      if (handler) {
        lock.unlock();
        handler(MessageContext(0, memory::SafeDataBuffer(data)));
        lock.lock();
      }

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

  void attach_framer_callback() {
    if (!framer) return;
    framer->on_message([this](memory::ConstByteSpan msg) {
      std::unique_lock<std::shared_mutex> lock(mutex_);
      if (message_batch_handler_) {
        message_batch_queue_.emplace_back(0, memory::SafeDataBuffer(msg));
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

      MessageHandler handler = message_handler;
      if (handler) {
        lock.unlock();
        handler(MessageContext(0, memory::SafeDataBuffer(msg)));
      }
    });
  }

  void set_framer(std::unique_ptr<framer::IFramer> f) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    framer = std::move(f);
    if (framer && (message_handler || message_batch_handler_)) attach_framer_callback();
  }

  void on_message(MessageHandler handler) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    message_handler = std::move(handler);
    if (framer) attach_framer_callback();
  }

  void on_message_batch(BatchMessageHandler handler) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    message_batch_handler_ = std::move(handler);
    if (framer) attach_framer_callback();
  }

  config::SerialConfig build_config_locked() const {
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
    config.backpressure_threshold = backpressure_threshold;
    config.backpressure_strategy = backpressure_strategy;
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
bool Serial::send(std::string_view data) {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  if (impl_->channel && impl_->channel->is_connected()) {
    auto binary_view = base::safe_convert::string_to_bytes(data);
    impl_->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    return true;
  }
  return false;
}
bool Serial::send_line(std::string_view line) { return send(std::string(line) + "\n"); }
bool Serial::connected() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  return impl_->channel && impl_->channel->is_connected();
}

ChannelInterface& Serial::on_data(MessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_handler = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_data_batch(BatchMessageHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_batch_handler_ = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_connect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->connect_handler = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_disconnect(ConnectionHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->disconnect_handler = std::move(h);
  return *this;
}
ChannelInterface& Serial::on_error(ErrorHandler h) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
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
ChannelInterface& Serial::on_message_batch(BatchMessageHandler h) {
  impl_->on_message_batch(std::move(h));
  return *this;
}

ChannelInterface& Serial::auto_start(bool m) {
  impl_->auto_start_.store(m);
  if (impl_->auto_start_.load() && !impl_->started_.load()) start();
  return *this;
}

Serial& Serial::baud_rate(uint32_t b) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->baud_rate = b;
  return *this;
}
Serial& Serial::data_bits(int d) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->data_bits = d;
  return *this;
}
Serial& Serial::stop_bits(int s) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->stop_bits = s;
  return *this;
}
Serial& Serial::parity(const std::string& p) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->parity = p;
  return *this;
}
Serial& Serial::flow_control(const std::string& f) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->flow_control = f;
  return *this;
}
Serial& Serial::retry_interval(std::chrono::milliseconds i) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->retry_interval = i;
  if (impl_->channel) {
    auto ts = std::dynamic_pointer_cast<transport::Serial>(impl_->channel);
    if (ts) ts->set_retry_interval(static_cast<unsigned int>(i.count()));
  }
  return *this;
}

Serial& Serial::backpressure_threshold(size_t threshold) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->backpressure_threshold = threshold;
  return *this;
}

Serial& Serial::backpressure_strategy(base::constants::BackpressureStrategy strategy) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->backpressure_strategy = strategy;
  return *this;
}

config::SerialConfig Serial::build_config() const {
  std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
  return impl_->build_config_locked();
}

Serial& Serial::manage_external_context(bool m) {
  std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
  impl_->manage_external_context = m;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
