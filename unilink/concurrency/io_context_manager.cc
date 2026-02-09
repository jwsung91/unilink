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

#include "unilink/concurrency/io_context_manager.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include "unilink/diagnostics/logger.hpp"

namespace unilink {
namespace concurrency {

struct IoContextManager::Impl {
  using IoContext = boost::asio::io_context;
  using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;

  bool owns_context_{true};
  std::shared_ptr<IoContext> ioc_;
  std::unique_ptr<WorkGuard> work_guard_;
  std::thread io_thread_;
  std::atomic<bool> running_{false};
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  bool stopping_{false};

  Impl() = default;
  Impl(std::shared_ptr<IoContext> external_context) : owns_context_(false), ioc_(std::move(external_context)) {}
  Impl(IoContext& external_context)
      : owns_context_(false), ioc_(std::shared_ptr<IoContext>(&external_context, [](IoContext*) {})) {}
};

IoContextManager::IoContextManager() : impl_(std::make_unique<Impl>()) {}

IoContextManager::IoContextManager(std::shared_ptr<IoContext> external_context)
    : impl_(std::make_unique<Impl>(std::move(external_context))) {}

IoContextManager::IoContextManager(IoContext& external_context) : impl_(std::make_unique<Impl>(external_context)) {}

IoContextManager& IoContextManager::instance() {
  static IoContextManager instance;
  return instance;
}

boost::asio::io_context& IoContextManager::get_context() {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  if (!impl_->ioc_) {
    impl_->ioc_ = std::make_shared<IoContext>();
    impl_->owns_context_ = true;
  }
  return *impl_->ioc_;
}

void IoContextManager::start() {
  std::shared_ptr<IoContext> context;
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);

    if (!impl_->owns_context_ && impl_->ioc_) {
      if (impl_->ioc_->stopped()) {
        UNILINK_LOG_WARNING("io_context_manager", "start",
                            "External io_context is stopped. The external owner must restart/run it.");
      } else {
        UNILINK_LOG_DEBUG("io_context_manager", "start",
                          "IoContextManager using external context. Thread creation skipped.");
      }
      return;
    }

    impl_->cv_.wait(lock, [this] { return !impl_->stopping_; });

    if (impl_->running_) {
      UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContextManager already running, ignoring start call.");
      return;
    }

    if (impl_->io_thread_.joinable() && impl_->io_thread_.get_id() == std::this_thread::get_id()) {
      UNILINK_LOG_ERROR("io_context_manager", "start", "Cannot restart IoContextManager from within its own thread.");
      return;
    }

    if (!impl_->ioc_) {
      impl_->ioc_ = std::make_shared<IoContext>();
      impl_->owns_context_ = true;
    }

    if (impl_->ioc_->stopped()) {
      impl_->ioc_->restart();
    }
    impl_->work_guard_ = std::make_unique<Impl::WorkGuard>(impl_->ioc_->get_executor());
    context = impl_->ioc_;

    if (impl_->io_thread_.joinable()) {
      impl_->io_thread_.join();
    }

    impl_->io_thread_ = std::thread([this, context]() {
      try {
        UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContext thread started.");
        context->run();
        UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContext thread finished running.");
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("io_context_manager", "run", "Thread error: " + std::string(e.what()));
      }
      impl_->running_.store(false);
    });
    impl_->running_.store(true);
  }
}

void IoContextManager::stop() {
  std::thread worker;
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);

    // Serialize stop operations to prevent race conditions where a newer start()
    // could be interrupted by an older stop() finishing its join.
    impl_->cv_.wait(lock, [this] { return !impl_->stopping_; });

    if (!impl_->owns_context_ && impl_->ioc_) {
      return;
    }

    if (!impl_->running_.load() && !impl_->io_thread_.joinable()) {
      UNILINK_LOG_DEBUG("io_context_manager", "stop",
                        "IoContextManager not running or thread not joinable, ignoring stop call.");
      return;
    }

    impl_->stopping_ = true;

    if (impl_->work_guard_) {
      impl_->work_guard_.reset();
    }

    if (impl_->ioc_ && impl_->owns_context_) {
      impl_->ioc_->stop();
    }

    if (impl_->io_thread_.joinable()) {
      if (impl_->io_thread_.get_id() != std::this_thread::get_id()) {
        worker = std::move(impl_->io_thread_);
      } else {
        UNILINK_LOG_ERROR("io_context_manager", "stop",
                          "Cannot join IoContext thread from within itself. Skipping join.");
        impl_->stopping_ = false;
        impl_->cv_.notify_all();
        return;
      }
    }
  }

  if (worker.joinable()) {
    UNILINK_LOG_DEBUG("io_context_manager", "stop", "Joining IoContext thread.");
    try {
      worker.join();
    } catch (const std::system_error& e) {
      UNILINK_LOG_ERROR("io_context_manager", "stop", "Failed to join thread: " + std::string(e.what()));
    }
    UNILINK_LOG_DEBUG("io_context_manager", "stop", "IoContext thread joined.");
  }

  {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->stopping_ = false;

    // Explicitly update running state
    impl_->running_.store(false);
  }
  impl_->cv_.notify_all();
}

bool IoContextManager::is_running() const { return impl_->running_.load(); }

std::unique_ptr<boost::asio::io_context> IoContextManager::create_independent_context() {
  return std::make_unique<IoContext>();
}

IoContextManager::IoContextManager(IoContextManager&& other) noexcept : impl_(std::move(other.impl_)) {}

IoContextManager& IoContextManager::operator=(IoContextManager&& other) noexcept {
  if (this != &other) {
    try {
      stop();
      if (impl_ && impl_->io_thread_.joinable() && impl_->io_thread_.get_id() != std::this_thread::get_id()) {
        impl_->io_thread_.join();
      }
    } catch (...) {
    }

    impl_ = std::move(other.impl_);
  }
  return *this;
}

IoContextManager::~IoContextManager() {
  try {
    stop();
    if (impl_ && impl_->io_thread_.joinable() && impl_->io_thread_.get_id() != std::this_thread::get_id()) {
      impl_->io_thread_.join();
    }
  } catch (...) {
  }
}

}  // namespace concurrency
}  // namespace unilink
