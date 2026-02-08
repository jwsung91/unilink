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

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/socket_base.hpp>
#include <cstddef>
#include <functional>

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
 * @brief An interface abstracting Boost.Asio's tcp::socket for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class UNILINK_API TcpSocketInterface {
 public:
  virtual ~TcpSocketInterface() = default;

  virtual void async_read_some(const boost::asio::mutable_buffer& buffer,
                               std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;
  virtual void async_write(const boost::asio::const_buffer& buffer,
                           std::function<void(const boost::system::error_code&, std::size_t)> handler) = 0;
  virtual void shutdown(boost::asio::socket_base::shutdown_type what, boost::system::error_code& ec) = 0;
  virtual void close(boost::system::error_code& ec) = 0;
  virtual boost::asio::ip::tcp::endpoint remote_endpoint(boost::system::error_code& ec) const = 0;
};

}  // namespace interface
}  // namespace unilink
