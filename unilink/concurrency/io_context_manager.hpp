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

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "unilink/base/platform.hpp"
#include "unilink/base/visibility.hpp"

// Forward declare boost components
namespace boost {
namespace asio {
class io_context;
}  // namespace asio
}  // namespace boost

namespace unilink {
namespace concurrency {

/**
 * Global io_context manager
 * All Transports share one io_context for improved memory efficiency
 * Added independent context creation functionality for test isolation
 */
class UNILINK_API IoContextManager {
 public:
  using IoContext = boost::asio::io_context;

  // Singleton instance access
  static IoContextManager& instance();

  IoContextManager();
  explicit IoContextManager(std::shared_ptr<IoContext> external_context);
  explicit IoContextManager(IoContext& external_context);

  // Return io_context reference (existing functionality)
  IoContext& get_context();

  // Start/stop io_context (existing functionality)
  void start();
  void stop();

  // Check status (existing functionality)
  bool is_running() const;

  // 🆕 Create independent io_context (for test isolation)
  std::unique_ptr<IoContext> create_independent_context();

  // Automatic cleanup in destructor
  ~IoContextManager();

  IoContextManager(IoContextManager&& other) noexcept;
  IoContextManager& operator=(IoContextManager&& other) noexcept;

 private:
  IoContextManager(const IoContextManager&) = delete;
  IoContextManager& operator=(const IoContextManager&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace concurrency

// Compatibility alias while transitioning from legacy `common` namespace.
namespace common {
using IoContextManager = concurrency::IoContextManager;
}  // namespace common
}  // namespace unilink
