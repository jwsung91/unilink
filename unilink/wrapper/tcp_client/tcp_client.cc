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
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "unilink/config/tcp_client_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

namespace unilink {
namespace wrapper {

TcpClient::TcpClient(const std::string& host, uint16_t port) : host_(host), port_(port), channel_(nullptr) {
  // Channel will be created later at start() time
}

TcpClient::TcpClient(const std::string& host, uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
    : host_(host),
      port_(port),
      channel_(nullptr),
      external_ioc_(std::move(external_ioc)),
      use_external_context_(external_ioc_ != nullptr) {}

TcpClient::TcpClient(std::shared_ptr<interface::Channel> channel) : host_(""), port_(0), channel_(channel) {
  setup_internal_handlers();
}

TcpClient::~TcpClient() {
  // Force cleanup - regardless of auto_manage setting
  if (started_) {
    stop();
  }
  // Explicit cleanup of Channel resources
  if (channel_) {
    channel_.reset();
  }
}

void TcpClient::start() {
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
    external_thread_ = std::thread([ioc = external_ioc_]() {
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard(ioc->get_executor());
      ioc->run();
    });
  }
  started_ = true;
}

void TcpClient::stop() {
  if (!started_ || !channel_) return;

  channel_->stop();
  // Brief wait for async operations to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  channel_.reset();
  started_ = false;

  if (use_external_context_ && manage_external_context_) {
    if (external_ioc_) {
      external_ioc_->stop();
    }
    if (external_thread_.joinable()) {
      external_thread_.join();
    }
  }
}

void TcpClient::send(const std::string& data) {
  if (is_connected() && channel_) {
    auto binary_data = common::safe_convert::string_to_uint8(data);
    channel_->async_write_copy(binary_data.data(), binary_data.size());
  }
}

void TcpClient::send_line(const std::string& line) { send(line + "\n"); }

bool TcpClient::is_connected() const { return channel_ && channel_->is_connected(); }

ChannelInterface& TcpClient::on_data(DataHandler handler) {
  data_handler_ = std::move(handler);
  if (channel_) {
    setup_internal_handlers();
  }
  return *this;
}

ChannelInterface& TcpClient::on_connect(ConnectHandler handler) {
  connect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpClient::on_disconnect(DisconnectHandler handler) {
  disconnect_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpClient::on_error(ErrorHandler handler) {
  error_handler_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpClient::auto_manage(bool manage) {
  auto_manage_ = manage;
  if (auto_manage_ && !started_) {
    start();
  }
  return *this;
}

void TcpClient::set_retry_interval(std::chrono::milliseconds interval) {
  retry_interval_ = interval;

  // If channel is already created, update its retry interval
  if (channel_) {
    // Cast to transport::TcpClient and set retry interval
    auto transport_client = std::dynamic_pointer_cast<transport::TcpClient>(channel_);
    if (transport_client) {
      transport_client->set_retry_interval(static_cast<unsigned int>(interval.count()));
    }
  }
}

void TcpClient::set_max_retries(int max_retries) { max_retries_ = max_retries; }

void TcpClient::set_connection_timeout(std::chrono::milliseconds timeout) { connection_timeout_ = timeout; }

void TcpClient::set_manage_external_context(bool manage) { manage_external_context_ = manage; }

void TcpClient::setup_internal_handlers() {
  if (!channel_) return;

  // Convert byte data to string and pass it
  channel_->on_bytes([this](const uint8_t* p, size_t n) {
    if (data_handler_) {
      std::string data = common::safe_convert::uint8_to_string(p, n);
      data_handler_(data);
    }
  });

  // Handle state changes
  channel_->on_state([this](base::LinkState state) { notify_state_change(state); });
}

void TcpClient::notify_state_change(base::LinkState state) {
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

}  // namespace wrapper
}  // namespace unilink
