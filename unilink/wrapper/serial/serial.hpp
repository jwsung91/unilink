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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "unilink/base/visibility.hpp"
#include "unilink/config/serial_config.hpp"
#include "unilink/wrapper/ichannel.hpp"

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace unilink {

namespace interface {
class Channel;
}

namespace wrapper {

class UNILINK_API Serial : public ChannelInterface {
 public:
  Serial(const std::string& device, uint32_t baud_rate);
  Serial(const std::string& device, uint32_t baud_rate, std::shared_ptr<boost::asio::io_context> external_ioc);
  explicit Serial(std::shared_ptr<interface::Channel> channel);
  ~Serial() override;

  // IChannel implementation
  void start() override;
  void stop() override;
  void send(std::string_view data) override;
  void send_line(std::string_view line) override;
  bool is_connected() const override;

  ChannelInterface& on_data(DataHandler handler) override;
  ChannelInterface& on_bytes(BytesHandler handler) override;
  ChannelInterface& on_connect(ConnectHandler handler) override;
  ChannelInterface& on_disconnect(DisconnectHandler handler) override;
  ChannelInterface& on_error(ErrorHandler handler) override;

  ChannelInterface& auto_manage(bool manage = true) override;

  // Serial-specific methods
  void set_baud_rate(uint32_t baud_rate);
  void set_data_bits(int data_bits);
  void set_stop_bits(int stop_bits);
  void set_parity(const std::string& parity);
  void set_flow_control(const std::string& flow_control);
  void set_retry_interval(std::chrono::milliseconds interval);
  // Expose mapped config for testing/inspection
  config::SerialConfig build_config() const;
  void set_manage_external_context(bool manage);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace wrapper
}  // namespace unilink
