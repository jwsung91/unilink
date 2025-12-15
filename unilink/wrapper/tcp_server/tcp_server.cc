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

#include "unilink/wrapper/tcp_server/tcp_server.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {
namespace wrapper {

TcpServer::TcpServer(uint16_t port) : port_(port), channel_(nullptr) {
  // Channel will be created later at start() time
}

TcpServer::TcpServer(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
    : port_(port),
      channel_(nullptr),
      external_ioc_(std::move(external_ioc)),
      use_external_context_(external_ioc_ != nullptr) {}

TcpServer::TcpServer(std::shared_ptr<interface::Channel> channel) : port_(0), channel_(channel) {
  setup_internal_handlers();
}

TcpServer::~TcpServer() {
  try {
    stop();
  } catch (...) {
    // Suppress all exceptions during cleanup to keep destruction noexcept
  }
}

void TcpServer::start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (started_) return;

  if (use_external_context_) {
    if (!external_ioc_) {
      throw std::runtime_error("External io_context is not set");
    }
    if (external_ioc_->stopped()) {
      external_ioc_->restart();
    }
    if (!external_guard_) {
      external_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          external_ioc_->get_executor());
    }
    if (!external_thread_.joinable()) {
      external_thread_ = std::thread([ioc = external_ioc_]() { ioc->run(); });
    }
  }

  if (!channel_) {
    // Use improved Factory
    config::TcpServerConfig config;
    config.port = port_;
    config.enable_port_retry = port_retry_enabled_;
    config.max_port_retries = max_port_retries_;
    config.port_retry_interval_ms = port_retry_interval_ms_;

    channel_ = factory::ChannelFactory::create(config, external_ioc_);
    setup_internal_handlers();

    // Apply stored client limit configuration
    if (client_limit_enabled_) {
      auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
      if (transport_server) {
        if (max_clients_ == 0) {
          transport_server->set_unlimited_clients();
        } else {
          transport_server->set_client_limit(max_clients_);
        }
      }
    }
  }

  channel_->start();
  started_ = true;
}

void TcpServer::stop() {
  std::shared_ptr<interface::Channel> local_channel;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_ && !channel_) {
      return;
    }

    // Detach callbacks to prevent use-after-free if background events arrive
    if (channel_) {
      channel_->on_bytes({});
      channel_->on_state({});
      channel_->on_backpressure({});
    }

    started_ = false;
    is_listening_ = false;
    local_channel = std::move(channel_);
  }

  if (local_channel) {
    local_channel->stop();
    // Allow async operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  if (use_external_context_) {
    if (external_guard_) {
      external_guard_->reset();
    }
    if (external_ioc_) {
      external_ioc_->stop();
    }
    if (external_thread_.joinable()) {
      external_thread_.join();
    }
  }
}

void TcpServer::send(const std::string& data) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (is_connected() && channel_) {
    auto binary_data = common::safe_convert::string_to_uint8(data);
    channel_->async_write_copy(binary_data.data(), binary_data.size());
  }
}

bool TcpServer::is_connected() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return channel_ && channel_->is_connected();
}

ChannelInterface& TcpServer::on_data(DataHandler handler) {
  on_data_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_connect(ConnectHandler handler) {
  on_connect_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_disconnect(DisconnectHandler handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_error(ErrorHandler handler) {
  on_error_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::auto_manage(bool manage) {
  auto_manage_ = manage;
  return *this;
}

void TcpServer::send_line(const std::string& line) { send(line + "\n"); }

// void ImprovedTcpServer::send_binary(const std::vector<uint8_t>& data) {
//     if (is_connected() && channel_) {
//         channel_->async_write_copy(data.data(), data.size());
//     }
// }

void TcpServer::setup_internal_handlers() {
  if (!channel_) return;

  channel_->on_bytes([this](const uint8_t* data, size_t size) { handle_bytes(data, size); });

  channel_->on_state([this](common::LinkState state) { handle_state(state); });

  // Set multi-client callbacks
  auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
  if (transport_server) {
    if (on_multi_connect_) {
      transport_server->on_multi_connect([this](size_t client_id, const std::string& client_info) {
        if (on_multi_connect_) {
          on_multi_connect_(client_id, client_info);
        }
      });
    }

    if (on_multi_data_) {
      transport_server->on_multi_data([this](size_t client_id, const std::string& data) {
        if (on_multi_data_) {
          on_multi_data_(client_id, data);
        }
      });
    }

    if (on_multi_disconnect_) {
      transport_server->on_multi_disconnect([this](size_t client_id) {
        if (on_multi_disconnect_) {
          on_multi_disconnect_(client_id);
        }
      });
    }
  }
}

void TcpServer::handle_bytes(const uint8_t* data, size_t size) {
  if (on_data_) {
    std::string str_data = common::safe_convert::uint8_to_string(data, size);
    on_data_(str_data);
  }
}

void TcpServer::handle_state(common::LinkState state) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // Update listening state based on server state
    if (state == common::LinkState::Listening) {
      is_listening_ = true;
    } else if (state == common::LinkState::Error || state == common::LinkState::Closed) {
      is_listening_ = false;
    }
  }

  switch (state) {
    case common::LinkState::Connected:
      // For TCP server, Connected state means a client connected
      // This should only be called when a client actually connects, not when server starts listening
      if (on_connect_) {
        on_connect_();
      }
      break;
    case common::LinkState::Closed:
      if (on_disconnect_) {
        on_disconnect_();
      }
      break;
    case common::LinkState::Error:
      if (on_error_) {
        on_error_("Connection error");
      }
      break;
    case common::LinkState::Listening:
      // Server is now listening - this is not a connection event
      // Do not call on_connect_ here
      break;
    default:
      break;
  }
}

// Multi-client support method implementations
void TcpServer::broadcast(const std::string& message) {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->broadcast(message);
    }
  }
}

void TcpServer::send_to_client(size_t client_id, const std::string& message) {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->send_to_client(client_id, message);
    }
  }
}

size_t TcpServer::get_client_count() const {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      return transport_server->get_client_count();
    }
  }
  return 0;
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      return transport_server->get_connected_clients();
    }
  }
  return {};
}

TcpServer& TcpServer::on_multi_connect(MultiClientConnectHandler handler) {
  on_multi_connect_ = std::move(handler);
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_connect([this](size_t client_id, const std::string& client_info) {
        if (on_multi_connect_) {
          on_multi_connect_(client_id, client_info);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::on_multi_data(MultiClientDataHandler handler) {
  on_multi_data_ = std::move(handler);
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_data([this](size_t client_id, const std::string& data) {
        if (on_multi_data_) {
          on_multi_data_(client_id, data);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::on_multi_disconnect(MultiClientDisconnectHandler handler) {
  on_multi_disconnect_ = std::move(handler);
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_disconnect([this](size_t client_id) {
        if (on_multi_disconnect_) {
          on_multi_disconnect_(client_id);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::enable_port_retry(bool enable, int max_retries, int retry_interval_ms) {
  // Store retry configuration for use when creating the channel
  port_retry_enabled_ = enable;
  max_port_retries_ = max_retries;
  port_retry_interval_ms_ = retry_interval_ms;

  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      // If channel already exists, we need to recreate it with new config
      // This is a limitation - retry settings should be set before start()
      // For now, we'll just store the settings
    }
  }
  return *this;
}

bool TcpServer::is_listening() const { return is_listening_; }

void TcpServer::set_client_limit(size_t max_clients) {
  max_clients_ = max_clients;
  client_limit_enabled_ = true;

  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->set_client_limit(max_clients);
    }
  }
}

void TcpServer::set_unlimited_clients() {
  client_limit_enabled_ = false;
  max_clients_ = 0;

  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->set_unlimited_clients();
    }
  }
}

}  // namespace wrapper
}  // namespace unilink
