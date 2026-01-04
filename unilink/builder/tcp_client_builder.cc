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

#include "unilink/builder/tcp_client_builder.hpp"

#include <boost/asio/io_context.hpp>

#include "unilink/builder/auto_initializer.hpp"
#include "unilink/base/constants.hpp"
#include "unilink/common/exceptions.hpp"
#include "unilink/common/input_validator.hpp"
#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

TcpClientBuilder::TcpClientBuilder(const std::string& host, uint16_t port)
    : host_(host), port_(port), auto_manage_(false), use_independent_context_(false), retry_interval_ms_(3000) {
  // Validate input parameters
  try {
    common::InputValidator::validate_host(host_);
    common::InputValidator::validate_port(port_);
  } catch (const common::ValidationException& e) {
    throw common::BuilderException("Invalid TCP client parameters: " + e.get_full_message(), "TcpClientBuilder",
                                   "constructor");
  }
}

std::unique_ptr<wrapper::TcpClient> TcpClientBuilder::build() {
  try {
    // Final validation before building
    common::InputValidator::validate_host(host_);
    common::InputValidator::validate_port(port_);
    common::InputValidator::validate_retry_interval(retry_interval_ms_);

    std::shared_ptr<boost::asio::io_context> external_ioc;
    // IoContext management
    if (use_independent_context_) {
      // Use independent IoContext (for test isolation)
      auto independent_context = common::IoContextManager::instance().create_independent_context();
      external_ioc = std::shared_ptr<boost::asio::io_context>(std::move(independent_context));
    } else {
      // Automatically initialize IoContextManager (default behavior)
      AutoInitializer::ensure_io_context_running();
    }

    auto client = external_ioc ? std::make_unique<wrapper::TcpClient>(host_, port_, external_ioc)
                               : std::make_unique<wrapper::TcpClient>(host_, port_);

    // Apply configuration with exception safety
    try {
      if (external_ioc) {
        client->set_manage_external_context(use_independent_context_);
      }

      // Set callbacks with exception safety
      if (on_data_) {
        client->on_data(on_data_);
      }

      if (on_connect_) {
        client->on_connect(on_connect_);
      }

      if (on_disconnect_) {
        client->on_disconnect(on_disconnect_);
      }

      if (on_error_) {
        client->on_error(on_error_);
      }

      // Set retry interval
      client->set_retry_interval(std::chrono::milliseconds(retry_interval_ms_));

      client->auto_manage(auto_manage_);

    } catch (const std::exception& e) {
      // If configuration fails, ensure client is properly cleaned up
      client.reset();
      throw common::BuilderException("Failed to configure TCP client: " + std::string(e.what()), "TcpClientBuilder",
                                     "build");
    }

    return client;

  } catch (const common::ValidationException& e) {
    throw common::BuilderException("Validation failed during TCP client build: " + e.get_full_message(),
                                   "TcpClientBuilder", "build");
  } catch (const std::bad_alloc& e) {
    throw common::BuilderException("Memory allocation failed during TCP client build: " + std::string(e.what()),
                                   "TcpClientBuilder", "build");
  } catch (const std::exception& e) {
    throw common::BuilderException("Unexpected error during TCP client build: " + std::string(e.what()),
                                   "TcpClientBuilder", "build");
  }
}

TcpClientBuilder& TcpClientBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_data(std::function<void(const std::string&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_connect(std::function<void()> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_disconnect(std::function<void()> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::on_error(std::function<void(const std::string&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

TcpClientBuilder& TcpClientBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

TcpClientBuilder& TcpClientBuilder::retry_interval(unsigned interval_ms) {
  try {
    common::InputValidator::validate_retry_interval(interval_ms);
    retry_interval_ms_ = interval_ms;
  } catch (const common::ValidationException& e) {
    throw common::BuilderException("Invalid retry interval: " + e.get_full_message(), "TcpClientBuilder",
                                   "retry_interval");
  }
  return *this;
}

}  // namespace builder
}  // namespace unilink
