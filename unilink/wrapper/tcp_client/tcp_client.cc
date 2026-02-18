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

#include "unilink/wrapper/tcp_client/tcp_client.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

namespace unilink {
namespace wrapper {

struct TcpClient::Impl {
  std::string host_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  // Start notification
  std::promise<bool> start_promise_;
  bool start_promise_fulfilled_{false};

  // Event handlers (Context based)
  MessageHandler data_handler_{nullptr};
  ConnectionHandler connect_handler_{nullptr};
  ConnectionHandler disconnect_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};

  // Configuration
  bool auto_manage_ = false;
  bool started_ = false;
  std::chrono::milliseconds retry_interval_{3000};
  int max_retries_ = -1;
  std::chrono::milliseconds connection_timeout_{5000};

  Impl(const std::string& host, uint16_t port) : host_(host), port_(port) {}

  Impl(const std::string& host, uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : host_(host),
        port_(port),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel) : host_(""), port_(0), channel_(std::move(channel)) {
    setup_internal_handlers();
  }

  ~Impl() {
    try { stop(); } catch (...) {}
  }

  std::future<bool> start() {
    if (started_) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }

    start_promise_ = std::promise<bool>();
    start_promise_fulfilled_ = false;

    if (!channel_) {
      config::TcpClientConfig config;
      config.host = host_;
      config.port = port_;
      config.retry_interval_ms = static_cast<unsigned int>(retry_interval_.count());
      config.max_retries = max_retries_;
      config.connection_timeout_ms = static_cast<unsigned>(connection_timeout_.count());
      channel_ = factory::ChannelFactory::create(config, external_ioc_);
      setup_internal_handlers();
    }

    channel_->start();
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      work_guard_.emplace(external_ioc_->get_executor());
      external_thread_ = std::thread([ioc = external_ioc_]() { ioc->run(); });
    }
    
    started_ = true;
    return start_promise_.get_future();
  }

  void stop() {
    if (!started_) return;

    if (channel_) {
      channel_->on_bytes(nullptr);
      channel_->on_state(nullptr);
      channel_->stop();
    }

    if (use_external_context_ && manage_external_context_ && external_thread_.joinable()) {
      if (work_guard_) work_guard_.reset();
      if (external_ioc_) external_ioc_->stop();
      external_thread_.join();
    }

    channel_.reset();
    started_ = false;

    if (!start_promise_fulfilled_) {
      try { start_promise_.set_value(false); } catch (...) {}
      start_promise_fulfilled_ = true;
    }
  }

  void send(std::string_view data) {
    if (is_connected() && channel_) {
      auto binary_view = common::safe_convert::string_to_bytes(data);
      channel_->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    }
  }

  bool is_connected() const {
    if (!channel_) return false;
    return channel_->is_connected();
  }

  void setup_internal_handlers() {
    if (!channel_) return;

    channel_->on_bytes([this](memory::ConstByteSpan data) {
      if (data_handler_) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler_(MessageContext(0, str_data));
      }
    });

    channel_->on_state([this](base::LinkState state) {
      switch (state) {
        case base::LinkState::Connected:
          if (!start_promise_fulfilled_) {
            start_promise_.set_value(true);
            start_promise_fulfilled_ = true;
          }
          if (connect_handler_) connect_handler_(ConnectionContext(0));
          break;
        case base::LinkState::Closed:
          if (disconnect_handler_) disconnect_handler_(ConnectionContext(0));
          break;
        case base::LinkState::Error:
          if (!start_promise_fulfilled_) {
            start_promise_.set_value(false);
            start_promise_fulfilled_ = true;
          }
          if (error_handler_) error_handler_(ErrorContext(ErrorCode::IoError, "Connection error occurred"));
          break;
        default: break;
      }
    });
  }
};

TcpClient::TcpClient(const std::string& h, uint16_t p) : pimpl_(std::make_unique<Impl>(h, p)) {}
TcpClient::TcpClient(const std::string& h, uint16_t p, std::shared_ptr<boost::asio::io_context> ioc) : pimpl_(std::make_unique<Impl>(h, p, ioc)) {}
TcpClient::TcpClient(std::shared_ptr<interface::Channel> ch) : pimpl_(std::make_unique<Impl>(ch)) {}
TcpClient::~TcpClient() = default;

std::future<bool> TcpClient::start() { return pimpl_->start(); }
void TcpClient::stop() { pimpl_->stop(); }
void TcpClient::send(std::string_view data) { pimpl_->send(data); }
void TcpClient::send_line(std::string_view line) { pimpl_->send(std::string(line) + "\n"); }
bool TcpClient::is_connected() const { return pimpl_->is_connected(); }

ChannelInterface& TcpClient::on_data(MessageHandler h) { pimpl_->data_handler_ = std::move(h); return *this; }
ChannelInterface& TcpClient::on_connect(ConnectionHandler h) { pimpl_->connect_handler_ = std::move(h); return *this; }
ChannelInterface& TcpClient::on_disconnect(ConnectionHandler h) { pimpl_->disconnect_handler_ = std::move(h); return *this; }
ChannelInterface& TcpClient::on_error(ErrorHandler h) { pimpl_->error_handler_ = std::move(h); return *this; }

ChannelInterface& TcpClient::auto_manage(bool m) {
  pimpl_->auto_manage_ = m;
  if (pimpl_->auto_manage_ && !pimpl_->started_) start();
  return *this;
}

TcpClient& TcpClient::set_retry_interval(std::chrono::milliseconds i) {
  pimpl_->retry_interval_ = i;
  if (pimpl_->channel_) {
    auto transport_client = std::dynamic_pointer_cast<transport::TcpClient>(pimpl_->channel_);
    if (transport_client) transport_client->set_retry_interval(static_cast<unsigned int>(i.count()));
  }
  return *this;
}

TcpClient& TcpClient::set_max_retries(int m) { pimpl_->max_retries_ = m; return *this; }
TcpClient& TcpClient::set_connection_timeout(std::chrono::milliseconds t) { pimpl_->connection_timeout_ = t; return *this; }
TcpClient& TcpClient::set_manage_external_context(bool m) { pimpl_->manage_external_context_ = m; return *this; }

}  // namespace wrapper
}  // namespace unilink
