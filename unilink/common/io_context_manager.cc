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

#include "unilink/common/io_context_manager.hpp"

#include "unilink/diagnostics/logger.hpp"

namespace unilink {
namespace common {

IoContextManager::IoContextManager() = default;

IoContextManager::IoContextManager(std::shared_ptr<IoContext> external_context)
    : owns_context_(false), ioc_(std::move(external_context)) {}

IoContextManager::IoContextManager(IoContext& external_context)
    : owns_context_(false), ioc_(std::shared_ptr<IoContext>(&external_context, [](IoContext*) {})) {}

IoContextManager& IoContextManager::instance() {
  static IoContextManager instance;
  return instance;
}

boost::asio::io_context& IoContextManager::get_context() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!ioc_) {
    ioc_ = std::make_shared<IoContext>();
    owns_context_ = true;
  }
  return *ioc_;
}

void IoContextManager::start() {
  std::shared_ptr<IoContext> context;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
      UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContextManager already running, ignoring start call.");
      return;
    }

    if (!ioc_) {
      ioc_ = std::make_shared<IoContext>();
      owns_context_ = true;
    }

    if (ioc_->stopped()) {
      ioc_->restart();
    }
    work_guard_ = std::make_unique<WorkGuard>(ioc_->get_executor());
    context = ioc_;  // Capture shared_ptr to io_context

    // If there was a previous thread, ensure it's joined before starting a new one
    if (io_thread_.joinable()) {
      io_thread_.join();
    }

    // Launch the io_context run in a new thread
    io_thread_ = std::thread([this, context]() {
      try {
        UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContext thread started.");
        context->run();
        UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContext thread finished running.");
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("io_context_manager", "run", "Thread error: " + std::string(e.what()));
      }
      running_.store(false);  // Mark as not running when thread exits
    });
    running_.store(true);  // Mark as running after thread is launched
  }
}

void IoContextManager::stop() {
  std::thread worker;  // Declare outside the lock
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_.load() && !io_thread_.joinable()) {
      UNILINK_LOG_DEBUG("io_context_manager", "stop",
                        "IoContextManager not running or thread not joinable, ignoring stop call.");
      return;
    }

    // Reset work_guard to allow io_context to stop once all tasks are done
    if (work_guard_) {
      work_guard_.reset();
    }

    // Stop io_context
    if (ioc_) {
      ioc_->stop();
    }

    // Move the thread handle out of the protected member to join it outside the lock
    if (io_thread_.joinable()) {
      worker = std::move(io_thread_);
    }

    running_.store(false);
  }  // Mutex is released here

  // Join the thread outside the lock to prevent deadlocks
  if (worker.joinable()) {
    UNILINK_LOG_DEBUG("io_context_manager", "stop", "Joining IoContext thread.");
    worker.join();
    UNILINK_LOG_DEBUG("io_context_manager", "stop", "IoContext thread joined.");
  }

  // After the thread has joined, perform final cleanup that might require the mutex again
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (owns_context_) {
      ioc_.reset();  // Reset owned io_context
    }
    // The work_guard_ should already be reset at the beginning of stop to unblock io_context.run()
    // If it's a shared context that is not owned, it should not be reset here.
    // If the context is owned, ioc_.reset() effectively cleans up the underlying Boost.Asio context.
  }
}

bool IoContextManager::is_running() const { return running_.load(); }

std::unique_ptr<boost::asio::io_context> IoContextManager::create_independent_context() {
  // Create independent io_context (for test isolation)
  // This context is completely separated from the global manager
  return std::make_unique<IoContext>();
}

IoContextManager::~IoContextManager() {
  try {
    stop();
  } catch (...) {
    // 소멸자에서는 예외를 무시
    // 로깅은 하지 않음 (소멸자에서 로깅은 위험할 수 있음)
  }
}

}  // namespace common
}  // namespace unilink
