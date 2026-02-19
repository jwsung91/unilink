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

#include "unilink/diagnostics/logger.hpp"

namespace unilink {
namespace concurrency {

IoContextManager::IoContextManager() {
  // Ensure Logger is initialized before IoContextManager finishes construction.
  // This guarantees Logger is destroyed AFTER IoContextManager.
  diagnostics::Logger::instance();
}

IoContextManager::IoContextManager(std::shared_ptr<IoContext> external_context)
    : owns_context_(false), ioc_(std::move(external_context)) {
  diagnostics::Logger::instance();
}

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
    std::unique_lock<std::mutex> lock(mutex_);

    // If we don't own the context, we treat start() as a check/no-op.
    if (!owns_context_ && ioc_) {
      if (ioc_->stopped()) {
        UNILINK_LOG_WARNING("io_context_manager", "start",
                            "External io_context is stopped. The external owner must restart/run it.");
      } else {
        UNILINK_LOG_DEBUG("io_context_manager", "start",
                          "IoContextManager using external context. Thread creation skipped.");
      }
      // running_ represents "internal thread running", so we leave it false.
      // Callers checking is_running() will get false, which is correct (we aren't running it).
      return;
    }

    cv_.wait(lock, [this] { return !stopping_; });

    if (running_) {
      UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContextManager already running, ignoring start call.");
      return;
    }

    // Prevent self-join if start() is called from within the IO thread
    if (io_thread_.joinable() && io_thread_.get_id() == std::this_thread::get_id()) {
      UNILINK_LOG_ERROR("io_context_manager", "start", "Cannot restart IoContextManager from within its own thread.");
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
        try {
          UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContext thread started.");
        } catch (...) {
        }
        context->run();
        try {
          UNILINK_LOG_DEBUG("io_context_manager", "start", "IoContext thread finished running.");
        } catch (...) {
        }
      } catch (const std::exception& e) {
        try {
          UNILINK_LOG_ERROR("io_context_manager", "run", "Thread error: " + std::string(e.what()));
        } catch (...) {
        }
      } catch (...) {
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

    // If we don't own the context, we just return.
    if (!owns_context_ && ioc_) {
      // running_ should already be false, but strict no-op is safest.
      return;
    }

    if (!running_.load() && !io_thread_.joinable()) {
      UNILINK_LOG_DEBUG("io_context_manager", "stop",
                        "IoContextManager not running or thread not joinable, ignoring stop call.");
      return;
    }

    stopping_ = true;

    // Reset work_guard to allow io_context to stop once all tasks are done
    if (work_guard_) {
      work_guard_.reset();
    }

    // Stop io_context only if we own it
    if (ioc_ && owns_context_) {
      ioc_->stop();
    }

    // Move the thread handle out of the protected member to join it outside the lock
    if (io_thread_.joinable()) {
      // Prevent self-join if stop() is called from within the IO thread
      if (io_thread_.get_id() != std::this_thread::get_id()) {
        worker = std::move(io_thread_);
      } else {
        UNILINK_LOG_ERROR("io_context_manager", "stop",
                          "Cannot join IoContext thread from within itself. Skipping join.");

        // REVERT STATE: Failed to stop.
        stopping_ = false;

        // NOTIFY: Wake up anyone waiting in start()
        cv_.notify_all();
        return;
      }
    }
  }  // Mutex is released here

  // Join the thread outside the lock to prevent deadlocks
  if (worker.joinable()) {
    try {
      UNILINK_LOG_DEBUG("io_context_manager", "stop", "Joining IoContext thread.");
    } catch (...) {
    }
    try {
      worker.join();
    } catch (...) {
    }
    try {
      UNILINK_LOG_DEBUG("io_context_manager", "stop", "IoContext thread joined.");
    } catch (...) {
    }
  }

  // After the thread has joined, perform final cleanup that might require the mutex again
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = false;

    // Ensure running flag is sync with reality
    if (!worker.joinable() && !io_thread_.joinable()) {
      running_.store(false);
    }
  }
  cv_.notify_all();
}

bool IoContextManager::is_running() const { return running_.load(); }

std::unique_ptr<boost::asio::io_context> IoContextManager::create_independent_context() {
  // Create independent io_context (for test isolation)
  // This context is completely separated from the global manager
  return std::make_unique<IoContext>();
}

IoContextManager::IoContextManager(IoContextManager&& other) noexcept
    : owns_context_(other.owns_context_),
      ioc_(std::move(other.ioc_)),
      work_guard_(std::move(other.work_guard_)),
      io_thread_(std::move(other.io_thread_)),
      running_(other.running_.load()),
      stopping_(other.stopping_) {
  other.owns_context_ = false;
  other.running_.store(false);
  other.stopping_ = false;
}

IoContextManager& IoContextManager::operator=(IoContextManager&& other) noexcept {
  if (this != &other) {
    try {
      stop();
      if (io_thread_.joinable() && io_thread_.get_id() != std::this_thread::get_id()) {
        io_thread_.join();
      }
    } catch (...) {
    }

    owns_context_ = other.owns_context_;
    ioc_ = std::move(other.ioc_);
    work_guard_ = std::move(other.work_guard_);
    io_thread_ = std::move(other.io_thread_);
    running_.store(other.running_.load());
    stopping_ = other.stopping_;

    other.owns_context_ = false;
    other.running_.store(false);
    other.stopping_ = false;
  }
  return *this;
}

IoContextManager::~IoContextManager() {
  try {
    stop();
    // Ensure thread is joined if it was left over (e.g. from failed self-stop)
    if (io_thread_.joinable() && io_thread_.get_id() != std::this_thread::get_id()) {
      io_thread_.join();
    }
  } catch (...) {
    // Ignore exceptions in destructor
  }
}

}  // namespace concurrency
}  // namespace unilink