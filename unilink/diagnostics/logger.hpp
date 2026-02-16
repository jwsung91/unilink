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
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "log_rotation.hpp"
#include "unilink/base/visibility.hpp"

#ifdef DEBUG
#undef DEBUG
#endif
#ifdef INFO
#undef INFO
#endif
#ifdef WARNING
#undef WARNING
#endif
#ifdef ERROR
#undef ERROR
#endif
#ifdef CRITICAL
#undef CRITICAL
#endif
#ifdef CALLBACK
#undef CALLBACK
#endif

namespace unilink {
namespace diagnostics {

/**
 * @brief Log severity levels
 */
enum class LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3, CRITICAL = 4 };

/**
 * @brief Log output destinations
 */
enum class LogOutput { CONSOLE = 0x01, FILE = 0x02, CALLBACK = 0x04 };

/**
 * @brief Log entry structure for async processing
 */
struct LogEntry {
  std::chrono::system_clock::time_point timestamp;
  LogLevel level;
  std::string component;
  std::string operation;
  std::string message;
  std::string formatted_message;

  LogEntry() : level(LogLevel::INFO) {}

  LogEntry(LogLevel lvl, std::string_view comp, std::string_view op, std::string_view msg)
      : timestamp(std::chrono::system_clock::now()), level(lvl), component(comp), operation(op), message(msg) {}
};

/**
 * @brief Async logging configuration
 */
struct AsyncLogConfig {
  size_t max_queue_size = 10000;                     // Maximum queue size
  size_t batch_size = 100;                           // Batch processing size
  std::chrono::milliseconds flush_interval{100};     // Flush interval
  std::chrono::milliseconds shutdown_timeout{5000};  // Shutdown timeout
  bool enable_backpressure = true;                   // Enable backpressure handling
  bool enable_batch_processing = true;               // Enable batch processing

  AsyncLogConfig() = default;

  AsyncLogConfig(size_t max_q, size_t batch, std::chrono::milliseconds interval)
      : max_queue_size(max_q), batch_size(batch), flush_interval(interval) {}
};

/**
 * @brief Async logging statistics
 */
struct AsyncLogStats {
  uint64_t total_logs{0};
  uint64_t dropped_logs{0};
  uint64_t queue_size{0};
  uint64_t max_queue_size_reached{0};
  uint64_t batch_count{0};
  uint64_t flush_count{0};
  std::chrono::system_clock::time_point start_time;

  AsyncLogStats() : start_time(std::chrono::system_clock::now()) {}

  void reset() {
    total_logs = 0;
    dropped_logs = 0;
    queue_size = 0;
    max_queue_size_reached = 0;
    batch_count = 0;
    flush_count = 0;
    start_time = std::chrono::system_clock::now();
  }

  double get_drop_rate() const {
    if (total_logs == 0) return 0.0;
    return static_cast<double>(dropped_logs) / static_cast<double>(total_logs);
  }

  std::chrono::milliseconds get_uptime() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time);
  }
};

/**
 * @brief Centralized logging system with async support
 *
 * Provides thread-safe, configurable logging with multiple output destinations,
 * async processing, batch operations, and performance optimizations for production use.
 */
class UNILINK_API Logger {
 public:
  using LogCallback = std::function<void(LogLevel level, const std::string& formatted_message)>;

  /**
   * @brief Get singleton instance
   */
  static Logger& instance();
  static Logger& default_logger();

  Logger();
  ~Logger();

  /**
   * @brief Set minimum log level
   * @param level Messages below this level will be ignored
   */
  void set_level(LogLevel level);

  /**
   * @brief Get current log level
   */
  LogLevel get_level() const;

  /**
   * @brief Enable/disable console output
   * @param enable True to enable console output
   */
  void set_console_output(bool enable);

  /**
   * @brief Set file output
   * @param filename Log file path (empty string to disable file output)
   */
  void set_file_output(const std::string& filename);

  /**
   * @brief Set file output with rotation
   * @param filename Log file path
   * @param config Rotation configuration
   */
  void set_file_output_with_rotation(const std::string& filename,
                                     const LogRotationConfig& config = LogRotationConfig{});

  /**
   * @brief Enable/disable async logging
   * @param enable True to enable async logging
   * @param config Async logging configuration
   */
  void set_async_logging(bool enable, const AsyncLogConfig& config = AsyncLogConfig{});

  /**
   * @brief Check if async logging is enabled
   */
  bool is_async_logging_enabled() const;

  /**
   * @brief Get async logging statistics
   */
  AsyncLogStats get_async_stats() const;

  /**
   * @brief Set log callback
   * @param callback Function to call for each log message
   */
  void set_callback(LogCallback callback);

  /**
   * @brief Set output destinations
   * @param outputs Bitwise OR of LogOutput flags
   */
  void set_outputs(int outputs);

  /**
   * @brief Enable/disable logging
   * @param enabled True to enable logging
   */
  void set_enabled(bool enabled);

  /**
   * @brief Check if logging is enabled
   */
  bool is_enabled() const;

  /**
   * @brief Set log format
   * @param format Format string with placeholders: {timestamp}, {level}, {component}, {operation}, {message}
   */
  void set_format(const std::string& format);

  /**
   * @brief Flush all outputs
   */
  void flush();

  // Main logging functions
  void log(LogLevel level, std::string_view component, std::string_view operation, std::string_view message);

  void debug(std::string_view component, std::string_view operation, std::string_view message);
  void info(std::string_view component, std::string_view operation, std::string_view message);
  void warning(std::string_view component, std::string_view operation, std::string_view message);
  void error(std::string_view component, std::string_view operation, std::string_view message);
  void critical(std::string_view component, std::string_view operation, std::string_view message);

 private:
  // Non-copyable, non-movable
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(Logger&&) = delete;

  mutable std::mutex mutex_;
  std::atomic<LogLevel> current_level_{LogLevel::INFO};
  std::atomic<bool> enabled_{true};
  std::atomic<int> outputs_{static_cast<int>(LogOutput::CONSOLE)};

  std::string format_string_{"{timestamp} [{level}] [{component}] [{operation}] {message}"};
  std::unique_ptr<std::ofstream> file_output_;
  LogCallback callback_;

  // Log rotation support
  std::unique_ptr<LogRotation> log_rotation_;
  std::string current_log_file_;

  // Async logging support
  std::atomic<bool> async_enabled_{false};
  AsyncLogConfig async_config_;
  AsyncLogStats async_stats_;

  // Threading for async logging
  std::thread worker_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> shutdown_requested_{false};

  // Queue management
  std::queue<LogEntry> log_queue_;
  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  mutable std::mutex stats_mutex_;

  std::string format_message(std::chrono::system_clock::time_point timestamp, LogLevel level,
                             std::string_view component, std::string_view operation, std::string_view message);
  std::string level_to_string(LogLevel level);
  std::string get_timestamp(std::chrono::system_clock::time_point timestamp);
  void write_to_console(const std::string& message);
  void write_to_file(const std::string& message);
  void call_callback(LogLevel level, const std::string& message);

  // Log rotation helper methods
  void check_and_rotate_log();
  void open_log_file(const std::string& filename);

  // Async logging helper methods
  void setup_async_logging(const AsyncLogConfig& config);
  void teardown_async_logging();
  void worker_loop();
  void process_batch(const std::vector<LogEntry>& batch);
  bool should_drop_log() const;
  size_t get_queue_size() const;
  void clear_queue();

  // Statistics helpers
  void update_stats_on_enqueue();
  void update_stats_on_drop();
  void update_stats_on_batch(size_t batch_size);
  void update_stats_on_flush();
};

/**
 * @brief Convenience macros for logging
 */
#define UNILINK_LOG_DEBUG(component, operation, message)                                                 \
  do {                                                                                                   \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::DEBUG) { \
      unilink::diagnostics::Logger::instance().debug(component, operation, message);                     \
    }                                                                                                    \
  } while (0)

#define UNILINK_LOG_INFO(component, operation, message)                                                 \
  do {                                                                                                  \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::INFO) { \
      unilink::diagnostics::Logger::instance().info(component, operation, message);                     \
    }                                                                                                   \
  } while (0)

#define UNILINK_LOG_WARNING(component, operation, message)                                                 \
  do {                                                                                                     \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::WARNING) { \
      unilink::diagnostics::Logger::instance().warning(component, operation, message);                     \
    }                                                                                                      \
  } while (0)

#define UNILINK_LOG_ERROR(component, operation, message)                                                 \
  do {                                                                                                   \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::ERROR) { \
      unilink::diagnostics::Logger::instance().error(component, operation, message);                     \
    }                                                                                                    \
  } while (0)

#define UNILINK_LOG_CRITICAL(component, operation, message)                                                 \
  do {                                                                                                      \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::CRITICAL) { \
      unilink::diagnostics::Logger::instance().critical(component, operation, message);                     \
    }                                                                                                       \
  } while (0)

/**
 * @brief Conditional logging macros (only evaluate message if level is enabled)
 */
#define UNILINK_LOG_DEBUG_IF(component, operation, message)                                              \
  do {                                                                                                   \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::DEBUG) { \
      UNILINK_LOG_DEBUG(component, operation, message);                                                  \
    }                                                                                                    \
  } while (0)

#define UNILINK_LOG_INFO_IF(component, operation, message)                                              \
  do {                                                                                                  \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::INFO) { \
      UNILINK_LOG_INFO(component, operation, message);                                                  \
    }                                                                                                   \
  } while (0)

/**
 * @brief Performance logging macros for expensive operations
 */
#define UNILINK_LOG_PERF_START(component, operation)                                                  \
  auto _perf_start_##operation =                                                                      \
      (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::DEBUG) \
          ? std::chrono::high_resolution_clock::now()                                                 \
          : std::chrono::high_resolution_clock::time_point()

#define UNILINK_LOG_PERF_END(component, operation)                                                                \
  do {                                                                                                            \
    if (unilink::diagnostics::Logger::instance().get_level() <= unilink::diagnostics::LogLevel::DEBUG) {          \
      auto _perf_end_##operation = std::chrono::high_resolution_clock::now();                                     \
      using _us_t = std::chrono::microseconds;                                                                    \
      auto _diff_##operation = _perf_end_##operation - _perf_start_##operation;                                   \
      auto _perf_duration_##operation = std::chrono::duration_cast<_us_t>(_diff_##operation).count();             \
      UNILINK_LOG_DEBUG(component, operation, "Duration: " + std::to_string(_perf_duration_##operation) + " μs"); \
    }                                                                                                             \
  } while (0)

}  // namespace diagnostics

}  // namespace unilink
