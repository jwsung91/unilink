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
#include <vector>

#include "unilink/base/common.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

namespace unilink {
namespace wrapper {

struct TcpClient::Impl {
  mutable std::mutex mutex_;
  std::string host_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;

  std::vector<std::promise<bool>> pending_promises_;
  std::atomic<bool> started_{false};

  MessageHandler data_handler_{nullptr};
  ConnectionHandler connect_handler_{nullptr};
  ConnectionHandler disconnect_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};

  bool auto_manage_ = false;
  std::chrono::milliseconds retry_interval_{3000};
  int max_retries_ = -1;
  std::chrono::milliseconds connection_timeout_{5000};

  Impl(const std::string& host, uint16_t port) : host_(host), port_(port), started_(false) {}

  Impl(const std::string& host, uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : host_(host),
        port_(port),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr),
        manage_external_context_(false),
        started_(false) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel) : host_(""), port_(0), channel_(std::move(channel)), started_(false) {
    setup_internal_handlers();
  }

  ~Impl() { try { stop(); } catch (...) {} }

  void fulfill_all(bool value) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : pending_promises_) { try { p.set_value(value); } catch (...) {} }
    pending_promises_.clear();
  }

  std::future<bool> start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (channel_ && channel_->is_connected()) {
      std::promise<bool> p; p.set_value(true); return p.get_future();
    }
    std::promise<bool> p;
    auto f = p.get_future();
    pending_promises_.push_back(std::move(p));
    if (started_) return f;

    if (!channel_) {
      config::TcpClientConfig config;
      config.host = host_; config.port = port_;
      config.retry_interval_ms = static_cast<unsigned int>(retry_interval_.count());
      config.max_retries = max_retries_;
      config.connection_timeout_ms = static_cast<unsigned>(connection_timeout_.count());
      channel_ = factory::ChannelFactory::create(config, external_ioc_);
      setup_internal_handlers();
    }
    channel_->start();
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(external_ioc_->get_executor());
      external_thread_ = std::thread([this, ioc = external_ioc_]() {
        try { 
          while (started_.load() && !ioc->stopped()) {
            if (ioc->run_one_for(std::chrono::milliseconds(50)) == 0) {
              std::this_thread::yield();
            }
          }
        } catch(...) {}
      });
    }
    started_ = true;
    return f;
  }

  void stop() {
    bool should_join = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!started_.load()) {
        for (auto& p : pending_promises_) { try { p.set_value(false); } catch(...) {} }
        pending_promises_.clear();
        return;
      }
      started_ = false;
      if (channel_) {
        channel_->on_bytes(nullptr);
        channel_->on_state(nullptr);
        channel_->stop();
      }
      if (use_external_context_ && manage_external_context_) {
        if (work_guard_) work_guard_.reset();
        if (external_ioc_) external_ioc_->stop();
        should_join = true;
      }
      for (auto& p : pending_promises_) { try { p.set_value(false); } catch(...) {} }
      pending_promises_.clear();
    }
    if (should_join && external_thread_.joinable()) {
      try { external_thread_.join(); } catch(...) {}
    }
    std::lock_guard<std::mutex> lock(mutex_);
    channel_.reset();
  }

  void send(std::string_view data) {
    if (channel_ && channel_->is_connected()) {
      auto binary_view = common::safe_convert::string_to_bytes(data);
      channel_->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    }
  }

  bool is_connected() const { return channel_ && channel_->is_connected(); }

  void setup_internal_handlers() {
    if (!channel_) return;
    
    // Explicitly do not use try-catch here to allow exceptions from handlers 
    // to propagate to transport layer for error handling (e.g., auto-reconnect)
    channel_->on_bytes([this](memory::ConstByteSpan data) {
      if (data_handler_) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler_(MessageContext(0, str_data));
      }
    });

    channel_->on_state([this](base::LinkState state) {
      if (state == base::LinkState::Connected) {
        fulfill_all(true);
        if (connect_handler_) connect_handler_(ConnectionContext(0));
      } else if (state == base::LinkState::Closed || state == base::LinkState::Error) {
        fulfill_all(false);
        if (state == base::LinkState::Closed && disconnect_handler_) {
          disconnect_handler_(ConnectionContext(0));
        } else if (state == base::LinkState::Error && error_handler_) {
          error_handler_(ErrorContext(ErrorCode::IoError, "Connection state error"));
        }
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
  if (pimpl_->auto_manage_ && !pimpl_->started_.load()) start();
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
