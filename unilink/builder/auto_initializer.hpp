#pragma once

#include <atomic>
#include <mutex>

#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Builder 패턴에서 자동으로 IoContextManager를 초기화하는 헬퍼 클래스
 *
 * 이 클래스는 Builder 패턴 사용 시 자동으로 IoContextManager를 시작하여
 * 사용자가 수동으로 초기화할 필요를 없애줍니다.
 */
class AutoInitializer {
 public:
  /**
   * @brief IoContextManager가 실행 중이 아니면 자동으로 시작
   *
   * 이 메서드는 스레드 안전하며, 여러 번 호출해도 안전합니다.
   * 이미 실행 중인 경우 아무것도 하지 않습니다.
   */
  static void ensure_io_context_running() {
    if (!common::IoContextManager::instance().is_running()) {
      std::lock_guard<std::mutex> lock(init_mutex_);
      // Double-check locking
      if (!common::IoContextManager::instance().is_running()) {
        common::IoContextManager::instance().start();
      }
    }
  }

  /**
   * @brief IoContextManager가 실행 중인지 확인
   */
  static bool is_io_context_running() { return common::IoContextManager::instance().is_running(); }

 private:
  static std::mutex init_mutex_;
  static std::atomic<bool> initialized_;
};

}  // namespace builder
}  // namespace unilink
