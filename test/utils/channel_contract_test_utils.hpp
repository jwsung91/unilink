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

#include <algorithm>
#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "unilink/common/common.hpp"

namespace unilink::test {

using namespace std::chrono_literals;

template <typename Pred>
bool wait_until(boost::asio::io_context& ioc, Pred&& pred, std::chrono::milliseconds timeout,
                std::chrono::milliseconds step = 5ms) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) return true;
    ioc.run_for(step);
    ioc.restart();
  }
  return pred();
}

template <typename Pred>
bool wait_until(Pred&& pred, std::chrono::milliseconds timeout, std::chrono::milliseconds step = 5ms) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) return true;
    std::this_thread::sleep_for(step);
  }
  return pred();
}

inline void pump_io(boost::asio::io_context& ioc, std::chrono::milliseconds duration,
                    std::chrono::milliseconds step = 5ms) {
  const auto deadline = std::chrono::steady_clock::now() + duration;
  while (std::chrono::steady_clock::now() < deadline) {
    ioc.run_for(step);
    ioc.restart();
  }
}

inline void pump_io(boost::asio::io_context& ioc, size_t steps, std::chrono::milliseconds step = 5ms) {
  for (size_t i = 0; i < steps; ++i) {
    ioc.run_for(step);
    ioc.restart();
  }
}

class CallbackRecorder {
 public:
  CallbackRecorder() = default;

  std::function<void(common::LinkState)> state_cb() {
    return [this](common::LinkState state) {
      CallbackGuard guard(*this);
      std::lock_guard<std::mutex> lock(mu_);
      states_.push_back(state);
    };
  }

  std::function<void(const uint8_t*, size_t)> bytes_cb() {
    return [this](const uint8_t*, size_t n) {
      CallbackGuard guard(*this);
      std::lock_guard<std::mutex> lock(mu_);
      bytes_calls_.push_back(n);
    };
  }

  size_t state_count(common::LinkState state) const {
    std::lock_guard<std::mutex> lock(mu_);
    return static_cast<size_t>(std::count(states_.begin(), states_.end(), state));
  }

  size_t bytes_call_count() const {
    std::lock_guard<std::mutex> lock(mu_);
    return bytes_calls_.size();
  }

  bool saw_overlap() const { return overlap_.load(); }

  void reset() {
    std::lock_guard<std::mutex> lock(mu_);
    states_.clear();
    bytes_calls_.clear();
    overlap_.store(false);
    in_callback_.clear();
  }

 private:
  class CallbackGuard {
   public:
    explicit CallbackGuard(CallbackRecorder& owner) : owner_(owner) {
      if (owner_.in_callback_.test_and_set(std::memory_order_acquire)) {
        owner_.overlap_.store(true);
      }
    }

    ~CallbackGuard() { owner_.in_callback_.clear(std::memory_order_release); }

   private:
    CallbackRecorder& owner_;
  };

  mutable std::mutex mu_;
  std::vector<common::LinkState> states_;
  std::vector<size_t> bytes_calls_;
  std::atomic_flag in_callback_ = ATOMIC_FLAG_INIT;
  std::atomic<bool> overlap_{false};
};

}  // namespace unilink::test
