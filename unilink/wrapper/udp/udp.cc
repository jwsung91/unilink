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

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/udp/udp.hpp"

namespace unilink {
namespace wrapper {

struct Udp::Impl {
  config::UdpConfig cfg;
  std::shared_ptr<interface::Channel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;

  // Start status notification
  std::promise<bool> start_promise_;
  bool start_promise_fulfilled_{false};

  // Event handlers (Context based)
  MessageHandler data_handler{nullptr};
  ConnectionHandler connect_handler{nullptr};
  ConnectionHandler disconnect_handler{nullptr};
  ErrorHandler error_handler{nullptr};

  bool auto_manage{false};
  bool started{false};

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

  std::future<bool> start() {
    if (started) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    start_promise_ = std::promise<bool>();
    start_promise_fulfilled_ = false;

    if (!channel) {
      channel = factory::ChannelFactory::create(cfg, external_ioc);
      setup_internal_handlers();
    }

    channel->start();
    if (use_external_context && manage_external_context && !external_thread.joinable()) {
      external_thread = std::thread([ioc = external_ioc]() {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard(ioc->get_executor());
        ioc->run();
      });
    }

    started = true;
    return start_promise_.get_future();
  }

  void stop() {
    if (!started) return;

    if (channel) {
      channel->on_bytes(nullptr);
      channel->on_state(nullptr);
      channel->stop();
    }

    if (use_external_context && manage_external_context && external_thread.joinable()) {
      if (external_ioc) external_ioc->stop();
      external_thread.join();
    }

    started = false;

    if (!start_promise_fulfilled_) {
      try {
        start_promise_.set_value(false);
      } catch (...) {
      }
      start_promise_fulfilled_ = true;
    }
  }

  void setup_internal_handlers() {
    if (!channel) return;

    channel->on_bytes([this](memory::ConstByteSpan data) {
      if (data_handler) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler(MessageContext(0, str_data));
      }
    });

    channel->on_state([this](base::LinkState state) {
      switch (state) {
        case base::LinkState::Connected:
          if (!start_promise_fulfilled_) {
            start_promise_.set_value(true);
            start_promise_fulfilled_ = true;
          }
          if (connect_handler) connect_handler(ConnectionContext(0));
          break;
        case base::LinkState::Closed:
          if (disconnect_handler) disconnect_handler(ConnectionContext(0));
          break;
        case base::LinkState::Error:
          if (!start_promise_fulfilled_) {
            start_promise_.set_value(false);
            start_promise_fulfilled_ = true;
          }
          if (error_handler) error_handler(ErrorContext(ErrorCode::IoError, "Connection error"));
          break;
        default:
          break;
      }
    });
  }
};

Udp::Udp(const config::UdpConfig& cfg) : impl_(std::make_unique<Impl>(cfg)) {}
Udp::Udp(const config::UdpConfig& cfg, std::shared_ptr<boost::asio::io_context> ioc)
    : impl_(std::make_unique<Impl>(cfg, ioc)) {}
Udp::Udp(std::shared_ptr<interface::Channel> ch) : impl_(std::make_unique<Impl>(ch)) {
  impl_->setup_internal_handlers();
}
Udp::~Udp() = default;

Udp::Udp(Udp&&) noexcept = default;
Udp& Udp::operator=(Udp&&) noexcept = default;

std::future<bool> Udp::start() { return impl_->start(); }
void Udp::stop() { impl_->stop(); }
void Udp::send(std::string_view data) {
  if (is_connected() && get_impl()->channel) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    get_impl()->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}
void Udp::send_line(std::string_view line) { send(std::string(line) + "\n"); }
bool Udp::is_connected() const { return get_impl()->channel && get_impl()->channel->is_connected(); }

ChannelInterface& Udp::on_data(MessageHandler h) {
  impl_->data_handler = std::move(h);
  return *this;
}
ChannelInterface& Udp::on_connect(ConnectionHandler h) {
  impl_->connect_handler = std::move(h);
  return *this;
}
ChannelInterface& Udp::on_disconnect(ConnectionHandler h) {
  impl_->disconnect_handler = std::move(h);
  return *this;
}
ChannelInterface& Udp::on_error(ErrorHandler h) {
  impl_->error_handler = std::move(h);
  return *this;
}

ChannelInterface& Udp::auto_manage(bool m) {
  impl_->auto_manage = m;
  if (impl_->auto_manage && !impl_->started) start();
  return *this;
}

void Udp::set_manage_external_context(bool manage) { impl_->manage_external_context = manage; }

}  // namespace wrapper
}  // namespace unilink
