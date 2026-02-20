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

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <memory>

#include "unilink/base/visibility.hpp"

namespace unilink {
namespace concurrency {

/**
 * Global io_context manager
 * All Transports share one io_context for improved memory efficiency
 */
class UNILINK_API IoContextManager {
 public:
  using IoContext = boost::asio::io_context;
  using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;

  static IoContextManager& instance();

  IoContextManager();
  explicit IoContextManager(std::shared_ptr<IoContext> external_context);
  explicit IoContextManager(IoContext& external_context);
  ~IoContextManager();

  // Move semantics
  IoContextManager(IoContextManager&&) noexcept;
  IoContextManager& operator=(IoContextManager&&) noexcept;

  // Non-copyable
  IoContextManager(const IoContextManager&) = delete;
  IoContextManager& operator=(const IoContextManager&) = delete;

  IoContext& get_context();
  void start();
  void stop();
  bool is_running() const;
  std::unique_ptr<IoContext> create_independent_context();

 private:
  struct Impl;
  const Impl* get_impl() const { return impl_.get(); }
  Impl* get_impl() { return impl_.get(); }
  std::unique_ptr<Impl> impl_;
};

}  // namespace concurrency

namespace common {
using IoContextManager = concurrency::IoContextManager;
}  // namespace common
}  // namespace unilink
