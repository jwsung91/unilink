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

#include <chrono>
#include <future>
#include <iostream>

#include "unilink/common/logger.hpp"

namespace unilink {
namespace common {

IoContextManager& IoContextManager::instance() {
  static IoContextManager instance;
  return instance;
}

boost::asio::io_context& IoContextManager::get_context() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!ioc_) {
    ioc_ = std::make_unique<IoContext>();
  }
  return *ioc_;
}

void IoContextManager::start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    return;
  }

  // Clean up previous state if exists
  if (io_thread_.joinable()) {
    try {
      io_thread_.join();
    } catch (...) {
      // Ignore join failure
    }
  }

  // Always create new io_context to ensure clean state
  ioc_ = std::make_unique<IoContext>();

  // Create work guard to keep io_context from becoming empty
  work_guard_ = std::make_unique<WorkGuard>(ioc_->get_executor());

  // Run io_context in separate thread
  io_thread_ = std::thread([this]() {
    try {
      ioc_->run();
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("io_context_manager", "run", "Thread error: " + std::string(e.what()));
    }
  });

  running_ = true;
}

void IoContextManager::stop() {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
      return;
    }

    // Release work guard to allow io_context to terminate naturally
    work_guard_.reset();

    // Stop io_context
    if (ioc_) {
      ioc_->stop();
    }

    // Wait for thread termination (safe way)
    if (io_thread_.joinable()) {
      try {
        // Attempt join with short timeout
        auto future = std::async(std::launch::async, [this]() { io_thread_.join(); });

        if (future.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout) {
          // Detach on timeout
          io_thread_.detach();
        }
      } catch (...) {
        // Detach on exception
        try {
          io_thread_.detach();
        } catch (...) {
          // Ignore if detach also fails
        }
      }
    }

    running_ = false;

    // Create completely new io_context to reset state
    ioc_.reset();
    work_guard_.reset();
  } catch (...) {
    // Ignore exceptions in stop()
    running_ = false;
    ioc_.reset();
    work_guard_.reset();
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
