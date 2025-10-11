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
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <thread>

namespace unilink {
namespace common {

/**
 * 전역 io_context 관리자
 * 모든 Transport가 하나의 io_context를 공유하여 메모리 효율성 향상
 * 테스트 격리를 위한 독립적인 컨텍스트 생성 기능 추가
 */
class IoContextManager {
 public:
  using IoContext = boost::asio::io_context;
  using WorkGuard = boost::asio::executor_work_guard<IoContext::executor_type>;

  // 싱글톤 인스턴스 접근
  static IoContextManager& instance();

  // io_context 참조 반환 (기존 기능)
  IoContext& get_context();

  // io_context 시작/중지 (기존 기능)
  void start();
  void stop();

  // 상태 확인 (기존 기능)
  bool is_running() const;

  // 🆕 독립적인 io_context 생성 (테스트 격리용)
  std::unique_ptr<IoContext> create_independent_context();

  // 소멸자에서 자동 정리
  ~IoContextManager();

 private:
  IoContextManager() = default;
  IoContextManager(const IoContextManager&) = delete;
  IoContextManager& operator=(const IoContextManager&) = delete;

  std::unique_ptr<IoContext> ioc_;
  std::unique_ptr<WorkGuard> work_guard_;
  std::thread io_thread_;
  std::atomic<bool> running_{false};
  mutable std::mutex mutex_;
};

}  // namespace common
}  // namespace unilink
