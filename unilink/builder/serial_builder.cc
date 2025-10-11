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

#include "unilink/builder/serial_builder.hpp"

#include "unilink/common/constants.hpp"
#include "unilink/common/exceptions.hpp"
#include "unilink/common/input_validator.hpp"
#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

SerialBuilder::SerialBuilder(const std::string& device, uint32_t baud_rate)
    : device_(device),
      baud_rate_(baud_rate),
      auto_start_(false),
      auto_manage_(false),
      use_independent_context_(false),
      retry_interval_ms_(common::constants::DEFAULT_RETRY_INTERVAL_MS) {
  // Validate input parameters
  try {
    common::InputValidator::validate_device_path(device_);
    common::InputValidator::validate_baud_rate(baud_rate_);
  } catch (const common::ValidationException& e) {
    throw common::BuilderException("Invalid Serial parameters: " + e.get_full_message(), "SerialBuilder",
                                   "constructor");
  }
}

std::unique_ptr<wrapper::Serial> SerialBuilder::build() {
  try {
    // Final validation before building
    common::InputValidator::validate_device_path(device_);
    common::InputValidator::validate_baud_rate(baud_rate_);
    common::InputValidator::validate_retry_interval(retry_interval_ms_);

    // IoContext management
    if (use_independent_context_) {
      // Use independent IoContext (for test isolation)
      // Create independent context through IoContextManager
      auto independent_context = common::IoContextManager::instance().create_independent_context();
      // Currently maintaining default implementation, can be extended in future for wrapper to accept independent
      // context
    } else {
      // Default behavior (Serial manages IoContext independently)
    }

    auto serial = std::make_unique<wrapper::Serial>(device_, baud_rate_);

    // Apply configuration with exception safety
    try {
      if (auto_start_) {
        serial->auto_start(true);
      }

      if (auto_manage_) {
        serial->auto_manage(true);
      }

      // Set callbacks with exception safety
      if (on_data_) {
        serial->on_data(on_data_);
      }

      if (on_connect_) {
        serial->on_connect(on_connect_);
      }

      if (on_disconnect_) {
        serial->on_disconnect(on_disconnect_);
      }

      if (on_error_) {
        serial->on_error(on_error_);
      }

      // Set retry interval
      serial->set_retry_interval(std::chrono::milliseconds(retry_interval_ms_));

    } catch (const std::exception& e) {
      // If configuration fails, ensure serial is properly cleaned up
      serial.reset();
      throw common::BuilderException("Failed to configure Serial: " + std::string(e.what()), "SerialBuilder", "build");
    }

    return serial;

  } catch (const common::ValidationException& e) {
    throw common::BuilderException("Validation failed during Serial build: " + e.get_full_message(), "SerialBuilder",
                                   "build");
  } catch (const std::bad_alloc& e) {
    throw common::BuilderException("Memory allocation failed during Serial build: " + std::string(e.what()),
                                   "SerialBuilder", "build");
  } catch (const std::exception& e) {
    throw common::BuilderException("Unexpected error during Serial build: " + std::string(e.what()), "SerialBuilder",
                                   "build");
  }
}

SerialBuilder& SerialBuilder::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

SerialBuilder& SerialBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

SerialBuilder& SerialBuilder::on_data(std::function<void(const std::string&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_connect(std::function<void()> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_disconnect(std::function<void()> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_error(std::function<void(const std::string&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

SerialBuilder& SerialBuilder::retry_interval(unsigned interval_ms) {
  try {
    common::InputValidator::validate_retry_interval(interval_ms);
    retry_interval_ms_ = interval_ms;
  } catch (const common::ValidationException& e) {
    throw common::BuilderException("Invalid retry interval: " + e.get_full_message(), "SerialBuilder",
                                   "retry_interval");
  }
  return *this;
}

}  // namespace builder
}  // namespace unilink
