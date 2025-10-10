#pragma once

#include <atomic>
#include <mutex>

#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Helper class that automatically initializes IoContextManager in Builder pattern
 *
 * This class automatically starts IoContextManager when using Builder pattern,
 * eliminating the need for manual initialization by users.
 */
class AutoInitializer {
 public:
  /**
   * @brief Automatically start IoContextManager if not running
   *
   * This method is thread-safe and can be called multiple times safely.
   * If already running, it does nothing.
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
   * @brief Check if IoContextManager is running
   */
  static bool is_io_context_running() { return common::IoContextManager::instance().is_running(); }

 private:
  static std::mutex init_mutex_;
  static std::atomic<bool> initialized_;
};

}  // namespace builder
}  // namespace unilink
