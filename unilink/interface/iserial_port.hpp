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

#include <boost/asio/serial_port_base.hpp>
#include <functional>
#include <string>

#include "unilink/base/platform.hpp"
#include "unilink/base/visibility.hpp"

// Forward declarations
namespace boost {
namespace asio {
class mutable_buffer;
class const_buffer;
}  // namespace asio
namespace system {
class error_code;
}
}  // namespace boost

namespace unilink {
namespace interface {

/**
 * @brief An interface abstracting Boost.Asio's serial_port for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class UNILINK_API SerialPortInterface {
 public:
  virtual ~SerialPortInterface() = default;

  virtual void open(const std::string& device, boost::system::error_code& ec) = 0;
  virtual bool is_open() const = 0;
  virtual void close(boost::system::error_code& ec) = 0;

  virtual void set_option(const boost::asio::serial_port_base::baud_rate& option, boost::system::error_code& ec) = 0;
  virtual void set_option(const boost::asio::serial_port_base::character_size& option, boost::system::error_code& ec) = 0;
  virtual void set_option(const boost::asio::serial_port_base::stop_bits& option, boost::system::error_code& ec) = 0;
  virtual void set_option(const boost::asio::serial_port_base::parity& option, boost::system::error_code& ec) = 0;
  virtual void set_option(const boost::asio::serial_port_base::flow_control& option, boost::system::error_code& ec) = 0;

  virtual void async_read_some(const boost::asio::mutable_buffer& buffer,
                               std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;
  virtual void async_write(const boost::asio::const_buffer& buffer,
                           std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;
};

}  // namespace interface
}  // namespace unilink
