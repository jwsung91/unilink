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

#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

namespace unilink {
namespace wrapper {

struct TcpServer::Impl {
  mutable std::mutex mutex_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;
  bool started_{false};
  bool auto_manage_{false};
  std::shared_ptr<boost::asio::io_context> external_ioc_;
  bool use_external_context_{false};
  bool manage_external_context_{false};
  std::thread external_thread_;

  // Port retry configuration
  bool port_retry_enabled_{false};
  int max_port_retries_{3};
  int port_retry_interval_ms_{1000};

  // Idle timeout configuration
  int idle_timeout_ms_{0};

  // Client limit configuration
  bool client_limit_enabled_{false};
  size_t max_clients_{0};

  // Server state tracking
  std::atomic<bool> is_listening_{false};

  // User callbacks
  DataHandler on_data_{nullptr};
  BytesHandler on_bytes_{nullptr};
  ConnectHandler on_connect_{nullptr};
  DisconnectHandler on_disconnect_{nullptr};
  ErrorHandler on_error_{nullptr};

  // Multi-client callbacks
  MultiClientConnectHandler on_multi_connect_{nullptr};
  MultiClientDataHandler on_multi_data_{nullptr};
  MultiClientDisconnectHandler on_multi_disconnect_{nullptr};

  // Send/broadcast 실패 시 on_error 호출 여부
  bool notify_send_failure_{false};

  explicit Impl(uint16_t port)
      : port_(port),
        channel_(nullptr),
        started_(false),
        auto_manage_(false),
        use_external_context_(false),
        manage_external_context_(false),
        port_retry_enabled_(false),
        max_port_retries_(3),
        port_retry_interval_ms_(1000),
        idle_timeout_ms_(0),
        client_limit_enabled_(false),
        max_clients_(0),
        is_listening_(false),
        notify_send_failure_(false) {}

  Impl(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
      : port_(port),
        channel_(nullptr),
        started_(false),
        auto_manage_(false),
        external_ioc_(std::move(external_ioc)),
        use_external_context_(external_ioc_ != nullptr),
        manage_external_context_(false),
        port_retry_enabled_(false),
        max_port_retries_(3),
        port_retry_interval_ms_(1000),
        idle_timeout_ms_(0),
        client_limit_enabled_(false),
        max_clients_(0),
        is_listening_(false),
        notify_send_failure_(false) {}

  explicit Impl(std::shared_ptr<interface::Channel> channel)
      : port_(0),
        channel_(std::move(channel)),
        started_(false),
        auto_manage_(false),
        use_external_context_(false),
        manage_external_context_(false),
        port_retry_enabled_(false),
        max_port_retries_(3),
        port_retry_interval_ms_(1000),
        idle_timeout_ms_(0),
        client_limit_enabled_(false),
        max_clients_(0),
        is_listening_(false),
        notify_send_failure_(false) {
    setup_internal_handlers();
  }

  ~Impl() {
    try {
      stop();
    } catch (...) {
      // Suppress all exceptions during cleanup
    }
  }

  void start() {
    std::lock_guard<std::mutex> lock(mutex_);
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
      // Use improved Factory
      config::TcpServerConfig config;
      config.port = port_;
      config.enable_port_retry = port_retry_enabled_;
      config.max_port_retries = max_port_retries_;
      config.port_retry_interval_ms = port_retry_interval_ms_;
      config.idle_timeout_ms = idle_timeout_ms_;

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
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      external_thread_ = std::thread([ioc = external_ioc_]() {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard(ioc->get_executor());
        ioc->run();
      });
    }
    started_ = true;
  }

  void stop() {
    std::shared_ptr<interface::Channel> local_channel;
    bool posted_transport_stop = false;
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

        // If underlying transport is TcpServer, prefer async stop to avoid callback reentrancy issues
        auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
        if (transport_server) {
          transport_server->request_stop();
          posted_transport_stop = true;
        }
      }

      started_ = false;
      is_listening_ = false;
      if (!posted_transport_stop) {
        local_channel = std::move(channel_);
      }
    }

    if (local_channel) {
      local_channel->stop();
    }

    if (use_external_context_) {
      if (manage_external_context_ && external_ioc_) {
        external_ioc_->stop();
      }
      if (manage_external_context_ && external_thread_.joinable()) {
        external_thread_.join();
      }
    }
  }

  void send(std::string_view data) {
    std::shared_ptr<interface::Channel> channel;
    bool connected = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      channel = channel_;
      connected = channel && channel->is_connected();
    }

    if (connected && channel) {
      auto binary_view = common::safe_convert::string_to_bytes(data);
      channel->async_write_copy(memory::ConstByteSpan(binary_view.first, binary_view.second));
    }
  }

  void setup_internal_handlers() {
    if (!channel_) return;

    channel_->on_bytes([this](memory::ConstByteSpan data) { handle_bytes(data); });

    channel_->on_state([this](base::LinkState state) { handle_state(state); });

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

  void handle_bytes(memory::ConstByteSpan data) {
    if (on_bytes_) {
      on_bytes_(data);
    }
    if (on_data_) {
      std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
      on_data_(str_data);
    }
  }

  void handle_state(base::LinkState state) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      // Update listening state based on server state
      if (state == base::LinkState::Listening) {
        is_listening_ = true;
      } else if (state == base::LinkState::Error || state == base::LinkState::Closed) {
        is_listening_ = false;
      }
    }

    switch (state) {
      case base::LinkState::Connected:
        // For TCP server, Connected state means a client connected
        // This should only be called when a client actually connects, not when server starts listening
        if (on_connect_) {
          on_connect_();
        }
        break;
      case base::LinkState::Closed:
        if (on_disconnect_) {
          on_disconnect_();
        }
        break;
      case base::LinkState::Error:
        if (on_error_) {
          on_error_("Connection error");
        }
        break;
      case base::LinkState::Listening:
        // Server is now listening - this is not a connection event
        // Do not call on_connect_ here
        break;
      default:
        break;
    }
  }
};

TcpServer::TcpServer(uint16_t port) : pimpl_(std::make_unique<Impl>(port)) {}

TcpServer::TcpServer(uint16_t port, std::shared_ptr<boost::asio::io_context> external_ioc)
    : pimpl_(std::make_unique<Impl>(port, std::move(external_ioc))) {}

TcpServer::TcpServer(std::shared_ptr<interface::Channel> channel) : pimpl_(std::make_unique<Impl>(channel)) {}

TcpServer::~TcpServer() = default;

void TcpServer::start() { pimpl_->start(); }

void TcpServer::stop() { pimpl_->stop(); }

void TcpServer::send(std::string_view data) { pimpl_->send(data); }

bool TcpServer::is_connected() const {
  std::lock_guard<std::mutex> lock(pimpl_->mutex_);
  return pimpl_->channel_ && pimpl_->channel_->is_connected();
}

ChannelInterface& TcpServer::on_data(DataHandler handler) {
  pimpl_->on_data_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_bytes(BytesHandler handler) {
  pimpl_->on_bytes_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_connect(ConnectHandler handler) {
  pimpl_->on_connect_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_disconnect(DisconnectHandler handler) {
  pimpl_->on_disconnect_ = std::move(handler);
  return *this;
}

ChannelInterface& TcpServer::on_error(ErrorHandler handler) {
  pimpl_->on_error_ = std::move(handler);
  return *this;
}

TcpServer& TcpServer::notify_send_failure(bool enable) {
  pimpl_->notify_send_failure_ = enable;
  return *this;
}

ChannelInterface& TcpServer::auto_manage(bool manage) {
  pimpl_->auto_manage_ = manage;
  if (pimpl_->auto_manage_ && !pimpl_->started_) {
    start();
  }
  return *this;
}

TcpServer& TcpServer::idle_timeout(int timeout_ms) {
  pimpl_->idle_timeout_ms_ = timeout_ms;
  return *this;
}

void TcpServer::send_line(std::string_view line) { send(std::string(line) + "\n"); }

bool TcpServer::broadcast(const std::string& message) {
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      bool ok = transport_server->broadcast(message);
      if (!ok && pimpl_->notify_send_failure_ && pimpl_->on_error_) {
        pimpl_->on_error_("Broadcast failed: no active clients");
      }
      return ok;
    }
  }
  return false;
}

bool TcpServer::send_to_client(size_t client_id, const std::string& message) {
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      bool ok = transport_server->send_to_client(client_id, message);
      if (!ok && pimpl_->notify_send_failure_ && pimpl_->on_error_) {
        pimpl_->on_error_("Send failed: client not found or disconnected");
      }
      return ok;
    }
  }
  return false;
}

size_t TcpServer::get_client_count() const {
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      return transport_server->get_client_count();
    }
  }
  return 0;
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      return transport_server->get_connected_clients();
    }
  }
  return {};
}

TcpServer& TcpServer::on_multi_connect(MultiClientConnectHandler handler) {
  pimpl_->on_multi_connect_ = std::move(handler);
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      transport_server->on_multi_connect([this](size_t client_id, const std::string& client_info) {
        if (pimpl_->on_multi_connect_) {
          pimpl_->on_multi_connect_(client_id, client_info);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::on_multi_data(MultiClientDataHandler handler) {
  pimpl_->on_multi_data_ = std::move(handler);
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      transport_server->on_multi_data([this](size_t client_id, const std::string& data) {
        if (pimpl_->on_multi_data_) {
          pimpl_->on_multi_data_(client_id, data);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::on_multi_disconnect(MultiClientDisconnectHandler handler) {
  pimpl_->on_multi_disconnect_ = std::move(handler);
  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      transport_server->on_multi_disconnect([this](size_t client_id) {
        if (pimpl_->on_multi_disconnect_) {
          pimpl_->on_multi_disconnect_(client_id);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::enable_port_retry(bool enable, int max_retries, int retry_interval_ms) {
  pimpl_->port_retry_enabled_ = enable;
  pimpl_->max_port_retries_ = max_retries;
  pimpl_->port_retry_interval_ms_ = retry_interval_ms;
  return *this;
}

bool TcpServer::is_listening() const { return pimpl_->is_listening_.load(); }

void TcpServer::set_manage_external_context(bool manage) { pimpl_->manage_external_context_ = manage; }

void TcpServer::set_client_limit(size_t max_clients) {
  pimpl_->max_clients_ = max_clients;
  pimpl_->client_limit_enabled_ = true;

  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      transport_server->set_client_limit(max_clients);
    }
  }
}

void TcpServer::set_unlimited_clients() {
  pimpl_->client_limit_enabled_ = false;
  pimpl_->max_clients_ = 0;

  if (pimpl_->channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(pimpl_->channel_);
    if (transport_server) {
      transport_server->set_unlimited_clients();
    }
  }
}

}  // namespace wrapper
}  // namespace unilink
