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

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace unilink {
namespace transport {

/**
 * @brief UDP Transport implementation
 */
class UNILINK_API UdpChannel : public interface::Channel, public std::enable_shared_from_this<UdpChannel> {
 public:
  static std::shared_ptr<UdpChannel> create(const config::UdpConfig& cfg);
  static std::shared_ptr<UdpChannel> create(const config::UdpConfig& cfg, boost::asio::io_context& ioc);
  ~UdpChannel() override;

  // Move semantics
  UdpChannel(UdpChannel&&) noexcept;
  UdpChannel& operator=(UdpChannel&&) noexcept;

  // Non-copyable
  UdpChannel(const UdpChannel&) = delete;
  UdpChannel& operator=(const UdpChannel&) = delete;

  // Channel implementation
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
  explicit UdpChannel(const config::UdpConfig& cfg);
  UdpChannel(const config::UdpConfig& cfg, boost::asio::io_context& ioc);

  struct Impl;
  const Impl* get_impl() const { return impl_.get(); }
  Impl* get_impl() { return impl_.get(); }
  std::unique_ptr<Impl> impl_;
};

}  // namespace transport
}  // namespace unilink
