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
#include <stdexcept>
#include <thread>

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
  std::string device;
  uint32_t baud_rate;
  std::shared_ptr<interface::Channel> channel;
  std::shared_ptr<boost::asio::io_context> external_ioc;
  bool use_external_context{false};
  bool manage_external_context{false};
  std::thread external_thread;

  // Start notification
  std::promise<bool> start_promise_;
  bool start_promise_fulfilled_{false};

  // Event handlers (Context based)
  MessageHandler data_handler{nullptr};
  ConnectionHandler connect_handler{nullptr};
  ConnectionHandler disconnect_handler{nullptr};
  ErrorHandler error_handler{nullptr};

  // Configuration
  bool auto_manage = false;
  bool started = false;
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
    try { stop(); } catch (...) {}
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
      channel = factory::ChannelFactory::create(build_config(), external_ioc);
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
      try { start_promise_.set_value(false); } catch (...) {}
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
        default: break;
      }
    });
  }

  config::SerialConfig build_config() const {
    config::SerialConfig config;
    config.device = device;
    config.baud_rate = baud_rate;
    config.char_size = static_cast<unsigned int>(data_bits);
    config.stop_bits = static_cast<unsigned int>(stop_bits);
    std::string p = to_lower(parity);
    if (p == "even") config.parity = config::SerialConfig::Parity::Even;
    else if (p == "odd") config.parity = config::SerialConfig::Parity::Odd;
    else config.parity = config::SerialConfig::Parity::None;
    
    std::string f = to_lower(flow_control);
    if (f == "software") config.flow = config::SerialConfig::Flow::Software;
    else if (f == "hardware") config.flow = config::SerialConfig::Flow::Hardware;
    else config.flow = config::SerialConfig::Flow::None;
    
    config.retry_interval_ms = static_cast<unsigned int>(retry_interval.count());
    return config;
  }
};

Serial::Serial(const std::string& d, uint32_t b) : pimpl_(std::make_unique<Impl>(d, b)) {}
Serial::Serial(const std::string& d, uint32_t b, std::shared_ptr<boost::asio::io_context> i) : pimpl_(std::make_unique<Impl>(d, b, i)) {}
Serial::Serial(std::shared_ptr<interface::Channel> ch) : pimpl_(std::make_unique<Impl>(ch)) { pimpl_->setup_internal_handlers(); }
Serial::~Serial() = default;

std::future<bool> Serial::start() { return pimpl_->start(); }
void Serial::stop() { pimpl_->stop(); }
void Serial::send(std::string_view data) {
  if (is_connected() && pimpl_->channel) {
    auto binary_view = common::safe_convert::string_to_bytes(data);
    pimpl_->channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
  }
}
void Serial::send_line(std::string_view line) { send(std::string(line) + "\n"); }
bool Serial::is_connected() const { return pimpl_->channel && pimpl_->channel->is_connected(); }

ChannelInterface& Serial::on_data(MessageHandler h) { pimpl_->data_handler = std::move(h); return *this; }
ChannelInterface& Serial::on_connect(ConnectionHandler h) { pimpl_->connect_handler = std::move(h); return *this; }
ChannelInterface& Serial::on_disconnect(ConnectionHandler h) { pimpl_->disconnect_handler = std::move(h); return *this; }
ChannelInterface& Serial::on_error(ErrorHandler h) { pimpl_->error_handler = std::move(h); return *this; }

ChannelInterface& Serial::auto_manage(bool m) {
  pimpl_->auto_manage = m;
  if (pimpl_->auto_manage && !pimpl_->started) start();
  return *this;
}

void Serial::set_baud_rate(uint32_t b) { pimpl_->baud_rate = b; }
void Serial::set_data_bits(int d) { pimpl_->data_bits = d; }
void Serial::set_stop_bits(int s) { pimpl_->stop_bits = s; }
void Serial::set_parity(const std::string& p) { pimpl_->parity = p; }
void Serial::set_flow_control(const std::string& f) { pimpl_->flow_control = f; }
void Serial::set_retry_interval(std::chrono::milliseconds i) {
  pimpl_->retry_interval = i;
  if (pimpl_->channel) {
    auto ts = std::dynamic_pointer_cast<transport::Serial>(pimpl_->channel);
    if (ts) ts->set_retry_interval(static_cast<unsigned int>(i.count()));
  }
}

config::SerialConfig Serial::build_config() const {
  return pimpl_->build_config();
}

void Serial::set_manage_external_context(bool m) { pimpl_->manage_external_context = m; }

}  // namespace wrapper
}  // namespace unilink
