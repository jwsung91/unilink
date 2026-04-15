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
#include "unilink/diagnostics/error_mapping.hpp"
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
  std::shared_ptr<bool> alive_marker_{std::make_shared<bool>(true)};

  MessageHandler data_handler_{nullptr};
  ConnectionHandler connect_handler_{nullptr};
  ConnectionHandler disconnect_handler_{nullptr};
  ErrorHandler error_handler_{nullptr};
  MessageHandler message_handler_{nullptr};

  std::unique_ptr<framer::IFramer> framer_{nullptr};

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

  explicit Impl(std::shared_ptr<interface::Channel> channel)
      : host_(""), port_(0), channel_(std::move(channel)), started_(false) {
    setup_internal_handlers();
  }

  ~Impl() {
    try {
      stop();
    } catch (...) {
    }
  }

  void fulfill_all(bool value) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : pending_promises_) {
      try {
        p.set_value(value);
      } catch (...) {
      }
    }
    pending_promises_.clear();
  }

  std::future<bool> start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (channel_ && channel_->is_connected()) {
      std::promise<bool> p;
      p.set_value(true);
      return p.get_future();
    }
    std::promise<bool> p;
    auto f = p.get_future();
    pending_promises_.push_back(std::move(p));
    if (started_) return f;

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
    started_ = true;
    channel_->start();
    if (use_external_context_ && manage_external_context_ && !external_thread_.joinable()) {
      if (external_ioc_ && external_ioc_->stopped()) {
        external_ioc_->restart();
      }
      work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
          external_ioc_->get_executor());
      external_thread_ = std::thread([this, ioc = external_ioc_]() {
        try {
          while (started_.load() && !ioc->stopped()) {
            if (ioc->run_one_for(std::chrono::milliseconds(50)) == 0) {
              std::this_thread::yield();
            }
          }
        } catch (...) {
        }
      });
    }
    return f;
  }

  void stop() {
    bool should_join = false;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!started_.load()) {
        for (auto& p : pending_promises_) {
          try {
            p.set_value(false);
          } catch (...) {
          }
        }
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
      for (auto& p : pending_promises_) {
        try {
          p.set_value(false);
        } catch (...) {
        }
      }
      pending_promises_.clear();
    }
    if (should_join && external_thread_.joinable()) {
      try {
        external_thread_.join();
      } catch (...) {
      }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    channel_.reset();
    if (framer_) framer_->reset();
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

    std::weak_ptr<bool> weak_alive = alive_marker_;

    // Explicitly do not use try-catch here to allow exceptions from handlers
    // to propagate to transport layer for error handling (e.g., auto-reconnect)
    channel_->on_bytes([this, weak_alive](memory::ConstByteSpan data) {
      if (weak_alive.expired()) return;

      // 1. Raw data handler
      MessageHandler data_handler;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        data_handler = data_handler_;
      }
      if (data_handler) {
        std::string str_data = common::safe_convert::uint8_to_string(data.data(), data.size());
        data_handler(MessageContext(0, str_data));
      }

      // 2. Framer integration
      std::lock_guard<std::mutex> lock(mutex_);
      if (framer_) {
        framer_->push_bytes(data);
      }
    });

    channel_->on_state([this, weak_alive](base::LinkState state) {
      if (weak_alive.expired()) return;
      ConnectionHandler connect_handler;
      ConnectionHandler disconnect_handler;
      ErrorHandler error_handler;
      std::shared_ptr<interface::Channel> channel_snapshot;

      if (state == base::LinkState::Connected) {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          for (auto& p : pending_promises_) {
            try {
              p.set_value(true);
            } catch (...) {
            }
          }
          pending_promises_.clear();
          connect_handler = connect_handler_;
        }
        if (connect_handler) connect_handler(ConnectionContext(0));
      } else if (state == base::LinkState::Closed || state == base::LinkState::Error) {
        {
          std::lock_guard<std::mutex> lock(mutex_);
          for (auto& p : pending_promises_) {
            try {
              p.set_value(false);
            } catch (...) {
            }
          }
          pending_promises_.clear();
          if (state == base::LinkState::Closed) {
            disconnect_handler = disconnect_handler_;
          } else {
            error_handler = error_handler_;
            channel_snapshot = channel_;
          }
        }
        if (state == base::LinkState::Closed && disconnect_handler) {
          disconnect_handler(ConnectionContext(0));
        } else if (state == base::LinkState::Error && error_handler) {
          bool handled = false;
          if (auto transport = std::dynamic_pointer_cast<transport::TcpClient>(channel_snapshot)) {
            if (auto info = transport->last_error_info()) {
              error_handler(diagnostics::to_error_context(*info));
              handled = true;
            }
          }
          if (!handled) {
            error_handler(ErrorContext(ErrorCode::IoError, "Connection error"));
          }
        }
      }
    });
  }

  void set_framer(std::unique_ptr<framer::IFramer> framer) {
    std::lock_guard<std::mutex> lock(mutex_);
    framer_ = std::move(framer);
    if (framer_ && message_handler_) {
      framer_->set_on_message([this](memory::ConstByteSpan msg) {
        MessageHandler handler;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          handler = message_handler_;
        }
        if (handler) {
          std::string str_msg = common::safe_convert::uint8_to_string(msg.data(), msg.size());
          handler(MessageContext(0, str_msg));
        }
      });
    }
  }

  void on_message(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    message_handler_ = std::move(handler);
    if (framer_) {
      framer_->set_on_message([this](memory::ConstByteSpan msg) {
        MessageHandler message_handler;
        {
          std::lock_guard<std::mutex> lock(mutex_);
          message_handler = message_handler_;
        }
        if (message_handler) {
          std::string str_msg = common::safe_convert::uint8_to_string(msg.data(), msg.size());
          message_handler(MessageContext(0, str_msg));
        }
      });
    }
  }
};

TcpClient::TcpClient(const std::string& h, uint16_t p) : impl_(std::make_unique<Impl>(h, p)) {}
TcpClient::TcpClient(const std::string& h, uint16_t p, std::shared_ptr<boost::asio::io_context> ioc)
    : impl_(std::make_unique<Impl>(h, p, ioc)) {}
TcpClient::TcpClient(std::shared_ptr<interface::Channel> ch) : impl_(std::make_unique<Impl>(ch)) {}
TcpClient::~TcpClient() = default;

TcpClient::TcpClient(TcpClient&&) noexcept = default;
TcpClient& TcpClient::operator=(TcpClient&&) noexcept = default;

std::future<bool> TcpClient::start() { return impl_->start(); }
void TcpClient::stop() { impl_->stop(); }
void TcpClient::send(std::string_view data) { impl_->send(data); }
void TcpClient::send_line(std::string_view line) { impl_->send(std::string(line) + "\n"); }
bool TcpClient::is_connected() const { return get_impl()->is_connected(); }

ChannelInterface& TcpClient::on_data(MessageHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->data_handler_ = std::move(h);
  return *this;
}
ChannelInterface& TcpClient::on_connect(ConnectionHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->connect_handler_ = std::move(h);
  return *this;
}
ChannelInterface& TcpClient::on_disconnect(ConnectionHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->disconnect_handler_ = std::move(h);
  return *this;
}
ChannelInterface& TcpClient::on_error(ErrorHandler h) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->error_handler_ = std::move(h);
  return *this;
}

ChannelInterface& TcpClient::framer(std::unique_ptr<framer::IFramer> f) {
  impl_->set_framer(std::move(f));
  return *this;
}
ChannelInterface& TcpClient::on_message(MessageHandler h) {
  impl_->on_message(std::move(h));
  return *this;
}

ChannelInterface& TcpClient::auto_manage(bool m) {
  impl_->auto_manage_ = m;
  if (impl_->auto_manage_ && !impl_->started_.load()) start();
  return *this;
}

TcpClient& TcpClient::retry_interval(std::chrono::milliseconds i) {
  impl_->retry_interval_ = i;
  if (impl_->channel_) {
    auto transport_client = std::dynamic_pointer_cast<transport::TcpClient>(impl_->channel_);
    if (transport_client) transport_client->set_retry_interval(static_cast<unsigned int>(i.count()));
  }
  return *this;
}

TcpClient& TcpClient::max_retries(int m) {
  impl_->max_retries_ = m;
  return *this;
}
TcpClient& TcpClient::connection_timeout(std::chrono::milliseconds t) {
  impl_->connection_timeout_ = t;
  return *this;
}
TcpClient& TcpClient::set_manage_external_context(bool m) {
  impl_->manage_external_context_ = m;
  return *this;
}

}  // namespace wrapper
}  // namespace unilink
