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

#include "unilink/common/logger.hpp"

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
  std::thread previous_thread;
  std::shared_ptr<IoContext> context;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
      return;
    }

    if (io_thread_.joinable()) {
      previous_thread = std::move(io_thread_);
    }

    if (!ioc_) {
      ioc_ = std::make_shared<IoContext>();
      owns_context_ = true;
    }

    if (ioc_->stopped()) {
      ioc_->restart();
    }
    work_guard_ = std::make_unique<WorkGuard>(ioc_->get_executor());
    context = ioc_;
    running_.store(true);
  }

  if (previous_thread.joinable()) {
    previous_thread.join();
  }

  // Run io_context in separate thread
  try {
    io_thread_ = std::thread([this, context]() {
      try {
        context->run();
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("io_context_manager", "run", "Thread error: " + std::string(e.what()));
      }
    });
  } catch (...) {
    running_.store(false);
    throw;
  }
}

void IoContextManager::stop() {
  std::thread worker;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_ && !io_thread_.joinable()) {
      return;
    }

    if (work_guard_) {
      work_guard_.reset();
    }

    if (ioc_) {
      ioc_->stop();
    }

    if (io_thread_.joinable()) {
      worker = std::move(io_thread_);
    }

    running_ = false;
  }

  if (worker.joinable()) {
    worker.join();
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (owns_context_) {
    ioc_.reset();
  }
  work_guard_.reset();
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
