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
#include <iostream>
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

  // Event handlers
  DataHandler data_handler_;
  BytesHandler bytes_handler_;
  ConnectHandler connect_handler_;
  DisconnectHandler disconnect_handler_;
  ErrorHandler error_handler_;

  // Configuration
  bool auto_manage_ = false;
  bool started_ = false;

  // TCP client specific configuration
  std::chrono::milliseconds retry_interval_{3000};  // 3 seconds default
  int max_retries_ = -1;                            // -1 means unlimited
  std::chrono::milliseconds connection_timeout_{5000};

  Impl(const std::string& host, uint16_t port) : host_(host), port_(port), channel_(nullptr) {}

  Impl(const std::string& host, uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : host_(host),
        port_(port),
        channel_(nullptr),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel) : host_(""), port_(0), channel_(channel) {
    setup_internal_handlers();
  }

  ~Impl() {
    try {
      stop();
    } catch (...) {
      // Suppress exceptions in destructor
    }
  }

  void start() {
    if (started_) return;

    if (use_external_context_) {
      if (!external_ioc_) {
        throw std::runtime_error("External io_context is not set");
      }
      if (manage_external_context_) {
        if (external_ioc_->stopped()) {
          external_ioc_->restart();
        }
      }
    }

    if (!channel_) {
      // Create Channel
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
  }

  void stop() {
    if (!started_) return;

    if (channel_) {
      // Clear handlers first
      channel_->on_bytes(nullptr);
      channel_->on_state(nullptr);
      channel_->stop();
    }

    if (use_external_context_ && manage_external_context_) {
      if (work_guard_) {
        work_guard_.reset();
      }
      if (external_ioc_) {
        external_ioc_->stop();
      }
      if (external_thread_.joinable()) {
        try {
          external_thread_.join();
        } catch (...) {
        }
      }
    }

    channel_.reset();
    started_ = false;
  }

  void send(std::string_view data) {
    if (is_connected() && channel_) {
      auto binary_view = common::safe_convert::string_to_bytes(data);
      channel_->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    }
  }

  bool is_connected() const { return channel_ && channel_->is_connected(); }

  void setup_internal_handlers() {
    if (!channel_) return;

    // Convert byte data to string and pass it
    channel_->on_bytes([this](memory::ConstByteSpan data) {
      if (bytes_handler_) {
        bytes_handler_(data);
      }
      if (data_handler_) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler_(str_data);
      }
    });

    // Handle state changes
    channel_->on_state([this](base::LinkState state) { notify_state_change(state); });
  }

  void notify_state_change(base::LinkState state) {
    switch (state) {
      case base::LinkState::Connected:
        if (connect_handler_) connect_handler_();
        break;
      case base::LinkState::Closed:
        if (disconnect_handler_) disconnect_handler_();
        break;
      case base::LinkState::Error:
        if (error_handler_) error_handler_("Connection error occurred");
        break;
      default:
        break;
    }
  }
};

TcpClient::TcpClient(const std::string& host, uint16_t port) : pimpl_(std::make_unique<Impl>(host, port)) {}

TcpClient::TcpClient(const std::string& host, uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
    : pimpl_(std::make_unique<Impl>(host, port, std::move(external_ioc))) {}

TcpClient::TcpClient(std::shared_ptr<interface::Channel> channel) : pimpl_(std::make_unique<Impl>(channel)) {}

TcpClient::~TcpClient() = default;

void TcpClient::start() { pimpl_->start(); }

void TcpClient::stop() { pimpl_->stop(); }

void TcpClient::send(std::string_view data) { pimpl_->send(data); }

void TcpClient::send_line(std::string_view line) { pimpl_->send(std::string(line) + "\n"); }

bool TcpClient::is_connected() const { return pimpl_->is_connected(); }

ChannelInterface& TcpClient::on_data(DataHandler handler) {
  pimpl_->data_handler_ = std::move(handler);
  if (pimpl_->channel_) {
    pimpl_->setup_internal_handlers();
  }
  return *this;
}

ChannelInterface& TcpClient::on_bytes(BytesHandler handler) {
  pimpl_->bytes_handler_ = std::move(handler);
  if (pimpl_->channel_) {
    pimpl_->setup_internal_handlers();
  }
  return *this;
}

ChannelInterface& TcpClient::on_connect(ConnectHandler handler) {
  pimpl_->connect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpClient::on_disconnect(DisconnectHandler handler) {
  pimpl_->disconnect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpClient::on_error(ErrorHandler handler) {
  pimpl_->error_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpClient::auto_manage(bool manage) {
  pimpl_->auto_manage_ = manage;
  if (pimpl_->auto_manage_ && !pimpl_->started_) {
    start();
  }
  return *this;
}

void TcpClient::set_retry_interval(std::chrono::milliseconds interval) {
  pimpl_->retry_interval_ = interval;

  // If channel is already created, update its retry interval
  if (pimpl_->channel_) {
    // Cast to transport::TcpClient and set retry interval
    auto transport_client = std::dynamic_pointer_cast<transport::TcpClient>(pimpl_->channel_);
    if (transport_client) {
      transport_client->set_retry_interval(static_cast<unsigned int>(interval.count()));
    }
  }
}

void TcpClient::set_max_retries(int max_retries) { pimpl_->max_retries_ = max_retries; }

void TcpClient::set_connection_timeout(std::chrono::milliseconds timeout) { pimpl_->connection_timeout_ = timeout; }

void TcpClient::set_manage_external_context(bool manage) { pimpl_->manage_external_context_ = manage; }

}  // namespace wrapper
}  // namespace unilink
