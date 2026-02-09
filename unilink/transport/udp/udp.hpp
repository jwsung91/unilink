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

#include <memory>
#include <vector>

#include "unilink/base/visibility.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/memory/memory_pool.hpp"

// Forward declarations to avoid Boost dependency in header
namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace unilink {
namespace transport {

using config::UdpConfig;
using interface::Channel;

class UNILINK_API UdpChannel : public Channel, public std::enable_shared_from_this<UdpChannel> {
 public:
  static std::shared_ptr<UdpChannel> create(const UdpConfig& cfg);
  static std::shared_ptr<UdpChannel> create(const UdpConfig& cfg, boost::asio::io_context& ioc);
  ~UdpChannel() override;

  void start() override;
  void stop() override;
  bool is_connected() const override;

  void async_write_copy(memory::ConstByteSpan data) override;
  void async_write_move(std::vector<uint8_t>&& data) override;
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) override;

  void on_bytes(OnBytes cb) override;
  void on_state(OnState cb) override;
  void on_backpressure(OnBackpressure cb) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  // Private constructors
  explicit UdpChannel(const UdpConfig& cfg);
  UdpChannel(const UdpConfig& cfg, boost::asio::io_context& ioc);
};

}  // namespace transport
}  // namespace unilink
