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

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "unilink/base/error_codes.hpp"
#include "unilink/memory/safe_span.hpp"

namespace unilink {
namespace wrapper {

/**
 * @brief Context for data/message related events
 */
class MessageContext {
 public:
  MessageContext(size_t client_id, std::string_view data, std::string client_info = "")
      : client_id_(client_id), data_(data), client_info_(std::move(client_info)) {}

  size_t client_id() const { return client_id_; }
  std::string_view data() const { return data_; }
  const std::string& client_info() const { return client_info_; }

  // Useful for servers (empty for point-to-point)
  const std::string& remote_address() const { return client_info_; }

 private:
  size_t client_id_;
  std::string_view data_;
  std::string client_info_;
};

/**
 * @brief Context for connection/disconnection events
 */
class ConnectionContext {
 public:
  ConnectionContext(size_t client_id, std::string client_info = "")
      : client_id_(client_id), client_info_(std::move(client_info)) {}

  size_t client_id() const { return client_id_; }
  const std::string& client_info() const { return client_info_; }

 private:
  size_t client_id_;
  std::string client_info_;
};

/**
 * @brief Context for error events
 */
class ErrorContext {
 public:
  ErrorContext(ErrorCode code, std::string_view message, std::optional<size_t> client_id = std::nullopt)
      : code_(code), message_(message), client_id_(client_id) {}

  ErrorCode code() const { return code_; }
  std::string_view message() const { return message_; }
  std::optional<size_t> client_id() const { return client_id_; }

 private:
  ErrorCode code_;
  std::string message_;
  std::optional<size_t> client_id_;
};

}  // namespace wrapper
}  // namespace unilink
