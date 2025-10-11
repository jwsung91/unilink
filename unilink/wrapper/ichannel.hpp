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

#include <functional>
#include <memory>
#include <string>

namespace unilink {
namespace wrapper {

// Common interface for all Wrapper communication classes
class ChannelInterface {
 public:
  using DataHandler = std::function<void(const std::string&)>;
  using ConnectHandler = std::function<void()>;
  using DisconnectHandler = std::function<void()>;
  using ErrorHandler = std::function<void(const std::string&)>;

  virtual ~ChannelInterface() = default;

  // Common methods
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void send(const std::string& data) = 0;
  virtual void send_line(const std::string& line) = 0;
  virtual bool is_connected() const = 0;

  // Event handler setup
  virtual ChannelInterface& on_data(DataHandler handler) = 0;
  virtual ChannelInterface& on_connect(ConnectHandler handler) = 0;
  virtual ChannelInterface& on_disconnect(DisconnectHandler handler) = 0;
  virtual ChannelInterface& on_error(ErrorHandler handler) = 0;

  // Convenience methods
  virtual ChannelInterface& auto_start(bool start = true) = 0;
  virtual ChannelInterface& auto_manage(bool manage = true) = 0;
};

}  // namespace wrapper
}  // namespace unilink
