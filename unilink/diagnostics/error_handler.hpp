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
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "error_types.hpp"
#include "unilink/base/visibility.hpp"

namespace unilink {
namespace diagnostics {

/**
 * @brief Centralized error handling system
 *
 * Provides thread-safe error reporting, statistics collection,
 * and callback-based error handling for the entire unilink library.
 */
class UNILINK_API ErrorHandler {
 public:
  using ErrorCallback = std::function<void(const ErrorInfo&)>;

  /**
   * @brief Get singleton instance
   */
  static ErrorHandler& instance();
  static ErrorHandler& default_handler();

  ErrorHandler();
  ~ErrorHandler();

  /**
   * @brief Report an error
   * @param error Error information to report
   */
  void report_error(const ErrorInfo& error);

  /**
   * @brief Register error callback
   * @param callback Function to call when errors occur
   */
  void register_callback(ErrorCallback callback);

  /**
   * @brief Unregister all callbacks
   */
  void clear_callbacks();

  /**
   * @brief Set minimum error level to report
   * @param level Minimum level (errors below this level are ignored)
   */
  void set_min_error_level(ErrorLevel level);

  /**
   * @brief Get current minimum error level
   */
  ErrorLevel get_min_error_level() const;

  /**
   * @brief Enable/disable error reporting
   * @param enabled True to enable, false to disable
   */
  void set_enabled(bool enabled);

  /**
   * @brief Check if error reporting is enabled
   */
  bool is_enabled() const;

  /**
   * @brief Get error statistics
   */
  ErrorStats get_error_stats() const;

  /**
   * @brief Reset error statistics
   */
  void reset_stats();

  /**
   * @brief Get errors by component
   * @param component Component name to filter by
   */
  std::vector<ErrorInfo> get_errors_by_component(const std::string& component) const;

  /**
   * @brief Get recent errors
   * @param count Maximum number of recent errors to return
   */
  std::vector<ErrorInfo> get_recent_errors(size_t count = 10) const;

  /**
   * @brief Check if component has any errors
   * @param component Component name to check
   */
  bool has_errors(const std::string& component) const;

  /**
   * @brief Get error count for specific component and level
   * @param component Component name
   * @param level Error level
   */
  size_t get_error_count(const std::string& component, ErrorLevel level) const;

 private:
  // Non-copyable, non-movable
  ErrorHandler(const ErrorHandler&) = delete;
  ErrorHandler& operator=(const ErrorHandler&) = delete;
  ErrorHandler(ErrorHandler&&) = delete;
  ErrorHandler& operator=(ErrorHandler&&) = delete;

  mutable std::mutex mutex_;
  std::vector<ErrorCallback> callbacks_;
  std::atomic<ErrorLevel> min_level_{ErrorLevel::INFO};
  std::atomic<bool> enabled_{true};

  // Statistics
  mutable std::mutex stats_mutex_;
  ErrorStats stats_;
  std::vector<ErrorInfo> recent_errors_;
  std::unordered_map<std::string, std::vector<ErrorInfo>> errors_by_component_;

  static constexpr size_t MAX_RECENT_ERRORS = 1000;

  void update_stats(const ErrorInfo& error);
  void notify_callbacks(const std::vector<ErrorCallback>& callbacks, const ErrorInfo& error);
  void add_to_recent_errors(const ErrorInfo& error);
  void add_to_component_errors(const ErrorInfo& error);
};

/**
 * @brief Convenience functions for common error reporting scenarios
 */
namespace error_reporting {

/**
 * @brief Report connection-related error
 * @param component Component name (e.g., "serial", "tcp_server")
 * @param operation Operation that failed (e.g., "connect", "bind")
 * @param ec Boost error code
 * @param retryable Whether this error is retryable
 */
UNILINK_API void report_connection_error(const std::string& component, const std::string& operation,
                                         const boost::system::error_code& ec, bool retryable = true);

/**
 * @brief Report communication-related error
 * @param component Component name
 * @param operation Operation that failed (e.g., "read", "write")
 * @param message Error message
 * @param retryable Whether this error is retryable
 */
UNILINK_API void report_communication_error(const std::string& component, const std::string& operation,
                                            const std::string& message, bool retryable = false);

/**
 * @brief Report configuration error
 * @param component Component name
 * @param operation Operation that failed (e.g., "validate", "apply")
 * @param message Error message
 */
UNILINK_API void report_configuration_error(const std::string& component, const std::string& operation,
                                            const std::string& message);

/**
 * @brief Report memory-related error
 * @param component Component name
 * @param operation Operation that failed (e.g., "allocate", "deallocate")
 * @param message Error message
 */
UNILINK_API void report_memory_error(const std::string& component, const std::string& operation,
                                     const std::string& message);

/**
 * @brief Report system-level error
 * @param component Component name
 * @param operation Operation that failed
 * @param message Error message
 * @param ec Optional Boost error code
 */
UNILINK_API void report_system_error(const std::string& component, const std::string& operation,
                                     const std::string& message,
                                     const boost::system::error_code& ec = boost::system::error_code{});

/**
 * @brief Report warning (non-critical issue)
 * @param component Component name
 * @param operation Operation
 * @param message Warning message
 */
UNILINK_API void report_warning(const std::string& component, const std::string& operation, const std::string& message);

/**
 * @brief Report informational message
 * @param component Component name
 * @param operation Operation
 * @param message Information message
 */
UNILINK_API void report_info(const std::string& component, const std::string& operation, const std::string& message);

}  // namespace error_reporting

}  // namespace diagnostics
}  // namespace unilink
