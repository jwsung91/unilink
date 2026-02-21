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

#include "logger.hpp"

#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string_view>
#include <thread>

namespace unilink {
namespace diagnostics {

struct Logger::Impl {
  mutable std::mutex mutex_;
  std::atomic<LogLevel> current_level_{LogLevel::INFO};
  std::atomic<bool> enabled_{true};
  std::atomic<int> outputs_{static_cast<int>(LogOutput::CONSOLE)};

  struct FormatPart {
    enum Type { LITERAL, TIMESTAMP, LEVEL, COMPONENT, OPERATION, MESSAGE };
    Type type;
    std::string value;  // Only used for LITERAL
  };

  struct LogFormat {
    std::string format_string;
    std::vector<FormatPart> parsed_format;
  };

  std::shared_ptr<LogFormat> log_format_;

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

  struct TimestampBuffer {
    char data[64];
    size_t length;
    std::string_view view() const { return {data, length}; }
  };

  Impl() { parse_format("{timestamp} [{level}] [{component}] [{operation}] {message}"); }

  ~Impl() {
    teardown_async_logging();
    flush();
  }

  void parse_format(const std::string& format) {
    auto new_format = std::make_shared<LogFormat>();
    new_format->format_string = format;

    size_t start = 0;
    size_t pos = 0;

    while ((pos = format.find('{', start)) != std::string::npos) {
      if (pos > start) {
        new_format->parsed_format.push_back({FormatPart::LITERAL, format.substr(start, pos - start)});
      }

      size_t end = format.find('}', pos);
      if (end == std::string::npos) {
        new_format->parsed_format.push_back({FormatPart::LITERAL, format.substr(pos)});
        start = format.length();
        break;
      }

      std::string placeholder = format.substr(pos + 1, end - pos - 1);
      if (placeholder == "timestamp") {
        new_format->parsed_format.push_back({FormatPart::TIMESTAMP, ""});
      } else if (placeholder == "level") {
        new_format->parsed_format.push_back({FormatPart::LEVEL, ""});
      } else if (placeholder == "component") {
        new_format->parsed_format.push_back({FormatPart::COMPONENT, ""});
      } else if (placeholder == "operation") {
        new_format->parsed_format.push_back({FormatPart::OPERATION, ""});
      } else if (placeholder == "message") {
        new_format->parsed_format.push_back({FormatPart::MESSAGE, ""});
      } else {
        new_format->parsed_format.push_back({FormatPart::LITERAL, format.substr(pos, end - pos + 1)});
      }

      start = end + 1;
    }

    if (start < format.length()) {
      new_format->parsed_format.push_back({FormatPart::LITERAL, format.substr(start)});
    }

    std::lock_guard<std::mutex> lock(mutex_);
    log_format_ = std::move(new_format);
  }

  void flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_output_ && file_output_->is_open()) {
      file_output_->flush();
    }
    std::cout.flush();
    std::cerr.flush();
  }

  std::string format_message(std::chrono::system_clock::time_point timestamp_val, LogLevel level,
                             std::string_view component, std::string_view operation, std::string_view message) {
    TimestampBuffer timestamp = get_timestamp(timestamp_val);
    std::string_view level_str = level_to_string(level);

    std::shared_ptr<LogFormat> current_format;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      current_format = log_format_;
    }

    std::string result;
    if (current_format) {
      result.reserve(current_format->format_string.length() + message.length() + 32);

      for (const auto& part : current_format->parsed_format) {
        switch (part.type) {
          case FormatPart::LITERAL:
            result.append(part.value);
            break;
          case FormatPart::TIMESTAMP:
            result.append(timestamp.view());
            break;
          case FormatPart::LEVEL:
            result.append(level_str);
            break;
          case FormatPart::COMPONENT:
            result.append(component);
            break;
          case FormatPart::OPERATION:
            result.append(operation);
            break;
          case FormatPart::MESSAGE:
            result.append(message);
            break;
        }
      }
    }

    return result;
  }

  std::string_view level_to_string(LogLevel level) const {
    switch (level) {
      case LogLevel::DEBUG:
        return "DEBUG";
      case LogLevel::INFO:
        return "INFO";
      case LogLevel::WARNING:
        return "WARNING";
      case LogLevel::ERROR:
        return "ERROR";
      case LogLevel::CRITICAL:
        return "CRITICAL";
    }
    return "UNKNOWN";
  }

  TimestampBuffer get_timestamp(std::chrono::system_clock::time_point timestamp) const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;

    static thread_local std::time_t last_t = -1;
    static thread_local char date_buf[32] = {0};

    if (time_t != last_t) {
      std::tm time_info{};
#if defined(_WIN32)
      ::localtime_s(&time_info, &time_t);
#else
      ::localtime_r(&time_t, &time_info);
#endif
      std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &time_info);
      last_t = time_t;
    }

    TimestampBuffer result;
    int len = std::snprintf(result.data, sizeof(result.data), "%s.%03d", date_buf, static_cast<int>(ms.count()));
    if (len > 0) {
      if (static_cast<size_t>(len) >= sizeof(result.data)) {
        result.length = sizeof(result.data) - 1;
      } else {
        result.length = static_cast<size_t>(len);
      }
    } else {
      result.length = 0;
    }
    return result;
  }

  void write_to_console(const std::string& message) const {
    if (message.find("[ERROR]") != std::string::npos || message.find("[CRITICAL]") != std::string::npos) {
      std::cerr << message << std::endl;
    } else {
#if defined(_WIN32)
      std::cout << message << std::endl;
#else
      std::cout << message << '\n';
#endif
    }
  }

  void write_to_file(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_output_ && file_output_->is_open()) {
      *file_output_ << message << '\n';
    }
  }

  void call_callback(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_) {
      try {
        callback_(level, message);
      } catch (const std::exception& e) {
        std::cerr << "Error in log callback: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Unknown error in log callback" << std::endl;
      }
    }
  }

  void check_and_rotate_log() {
    if (!log_rotation_ || current_log_file_.empty()) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    bool should_rotate = false;
    if (file_output_ && file_output_->is_open()) {
      std::streampos current_pos = file_output_->tellp();
      if (current_pos != static_cast<std::streampos>(-1) &&
          static_cast<size_t>(current_pos) >= log_rotation_->get_config().max_file_size_bytes) {
        should_rotate = true;
      }
    } else {
      if (log_rotation_->should_rotate(current_log_file_)) {
        should_rotate = true;
      }
    }

    if (!should_rotate) {
      return;
    }

    if (file_output_) {
      file_output_->flush();
      file_output_->close();
      file_output_.reset();
    }

    log_rotation_->rotate(current_log_file_);
    open_log_file(current_log_file_);
  }

  void open_log_file(const std::string& filename) {
    file_output_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (file_output_->is_open()) {
      outputs_.fetch_or(static_cast<int>(LogOutput::FILE));
    } else {
      file_output_.reset();
      std::cerr << "Failed to open log file: " << filename << std::endl;
    }
  }

  void setup_async_logging(const AsyncLogConfig& config) {
    teardown_async_logging();

    {
      std::lock_guard<std::mutex> lock(mutex_);
      async_config_ = config;
      async_stats_.reset();
      shutdown_requested_.store(false);
      running_.store(true);
      async_enabled_.store(true);
    }

    worker_thread_ = std::thread(&Impl::worker_loop, this);
  }

  void teardown_async_logging() {
    std::thread worker_to_join;
    bool notify_worker = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      async_enabled_.store(false);

      if (running_.load()) {
        shutdown_requested_.store(true);
        notify_worker = true;
      }

      if (worker_thread_.joinable()) {
        worker_to_join = std::move(worker_thread_);
      }
    }

    if (notify_worker) {
      queue_cv_.notify_all();
    }

    if (worker_to_join.joinable()) {
      worker_to_join.join();
    }

    running_.store(false);
    shutdown_requested_.store(false);
  }

  void worker_loop() {
    std::vector<LogEntry> batch;
    batch.reserve(async_config_.batch_size);

    auto last_flush = std::chrono::steady_clock::now();

    while (true) {
      std::unique_lock<std::mutex> lock(queue_mutex_);

      bool has_logs = queue_cv_.wait_for(lock, async_config_.flush_interval,
                                         [this] { return !log_queue_.empty() || shutdown_requested_.load(); });

      if (has_logs && !log_queue_.empty()) {
        size_t batch_size =
            async_config_.enable_batch_processing ? std::min(async_config_.batch_size, log_queue_.size()) : 1;

        batch.clear();
        batch.reserve(batch_size);

        for (size_t i = 0; i < batch_size && !log_queue_.empty(); ++i) {
          batch.push_back(std::move(log_queue_.front()));
          log_queue_.pop();
        }

        lock.unlock();

        if (!batch.empty()) {
          process_batch(batch);
          update_stats_on_batch(batch.size());
        }
      } else {
        lock.unlock();
      }

      auto now = std::chrono::steady_clock::now();
      if (now - last_flush >= async_config_.flush_interval) {
        flush();
        update_stats_on_flush();
        last_flush = now;
      }

      if (shutdown_requested_.load() && log_queue_.empty()) {
        break;
      }
    }

    flush();
    running_.store(false);
  }

  void process_batch(const std::vector<LogEntry>& batch) {
    for (const auto& entry : batch) {
      std::string formatted_message =
          format_message(entry.timestamp, entry.level, entry.component, entry.operation, entry.message);
      int current_outputs = outputs_.load();

      if (current_outputs & static_cast<int>(LogOutput::CONSOLE)) {
        write_to_console(formatted_message);
      }

      if (current_outputs & static_cast<int>(LogOutput::FILE)) {
        check_and_rotate_log();
        write_to_file(formatted_message);
      }

      if (current_outputs & static_cast<int>(LogOutput::CALLBACK)) {
        call_callback(entry.level, formatted_message);
      }
    }
  }

  bool should_drop_log() const {
    size_t current_size = get_queue_size();
    return current_size >= async_config_.max_queue_size;
  }

  size_t get_queue_size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return log_queue_.size();
  }

  void update_stats_on_enqueue() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    async_stats_.total_logs++;
  }

  void update_stats_on_drop() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    async_stats_.dropped_logs++;
  }

  void update_stats_on_batch(size_t /* batch_size */) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    async_stats_.batch_count++;
  }

  void update_stats_on_flush() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    async_stats_.flush_count++;
  }
};

Logger::Logger() : impl_(std::make_unique<Impl>()) {}

Logger::~Logger() = default;

Logger::Logger(Logger&&) noexcept = default;
Logger& Logger::operator=(Logger&&) noexcept = default;

Logger& Logger::default_logger() {
  static Logger* instance = new Logger();
  return *instance;
}

Logger& Logger::instance() { return default_logger(); }

void Logger::set_level(LogLevel level) { impl_->current_level_.store(level); }

LogLevel Logger::get_level() const { return get_impl()->current_level_.load(); }

void Logger::set_console_output(bool enable) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  if (enable) {
    impl_->outputs_.fetch_or(static_cast<int>(LogOutput::CONSOLE));
  } else {
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::CONSOLE));
  }
}

void Logger::set_file_output(const std::string& filename) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);

  if (filename.empty()) {
    impl_->file_output_.reset();
    impl_->log_rotation_.reset();
    impl_->current_log_file_.clear();
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::FILE));
  } else {
    impl_->open_log_file(filename);
  }
}

void Logger::set_file_output_with_rotation(const std::string& filename, const LogRotationConfig& config) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);

  if (filename.empty()) {
    impl_->file_output_.reset();
    impl_->log_rotation_.reset();
    impl_->current_log_file_.clear();
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::FILE));
  } else {
    impl_->log_rotation_ = std::make_unique<LogRotation>(config);
    impl_->current_log_file_ = filename;
    impl_->open_log_file(filename);
  }
}

void Logger::set_async_logging(bool enable, const AsyncLogConfig& config) {
  if (enable) {
    impl_->setup_async_logging(config);
  } else {
    impl_->teardown_async_logging();
  }
}

bool Logger::is_async_logging_enabled() const { return get_impl()->async_enabled_.load(); }

AsyncLogStats Logger::get_async_stats() const {
  std::lock_guard<std::mutex> lock(get_impl()->stats_mutex_);
  AsyncLogStats result = get_impl()->async_stats_;
  result.queue_size = get_impl()->get_queue_size();
  result.max_queue_size_reached = std::max(result.max_queue_size_reached, result.queue_size);
  return result;
}

void Logger::set_callback(LogCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->callback_ = std::move(callback);
  if (impl_->callback_) {
    impl_->outputs_.fetch_or(static_cast<int>(LogOutput::CALLBACK));
  } else {
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::CALLBACK));
  }
}

void Logger::set_outputs(int outputs) { impl_->outputs_.store(outputs); }

void Logger::set_enabled(bool enabled) { impl_->enabled_.store(enabled); }

bool Logger::is_enabled() const { return get_impl()->enabled_.load(); }

void Logger::set_format(const std::string& format) { impl_->parse_format(format); }

void Logger::flush() { impl_->flush(); }

void Logger::log(LogLevel level, std::string_view component, std::string_view operation, std::string_view message) {
  if (!get_impl()->enabled_.load() || level < get_impl()->current_level_.load()) {
    return;
  }

  if (get_impl()->async_enabled_.load()) {
    LogEntry entry(level, component, operation, message);
    impl_->update_stats_on_enqueue();

    if (get_impl()->async_config_.enable_backpressure && impl_->should_drop_log()) {
      impl_->update_stats_on_drop();
      return;
    }

    {
      std::lock_guard<std::mutex> lock(impl_->queue_mutex_);
      impl_->log_queue_.push(entry);
    }

    impl_->queue_cv_.notify_one();
    return;
  }

  std::string formatted_message =
      impl_->format_message(std::chrono::system_clock::now(), level, component, operation, message);
  int current_outputs = get_impl()->outputs_.load();

  if (current_outputs & static_cast<int>(LogOutput::CONSOLE)) {
    impl_->write_to_console(formatted_message);
  }

  if (current_outputs & static_cast<int>(LogOutput::FILE)) {
    impl_->check_and_rotate_log();
    impl_->write_to_file(formatted_message);
  }

  if (current_outputs & static_cast<int>(LogOutput::CALLBACK)) {
    impl_->call_callback(level, formatted_message);
  }
}

void Logger::debug(std::string_view component, std::string_view operation, std::string_view message) {
  log(LogLevel::DEBUG, component, operation, message);
}

void Logger::info(std::string_view component, std::string_view operation, std::string_view message) {
  log(LogLevel::INFO, component, operation, message);
}

void Logger::warning(std::string_view component, std::string_view operation, std::string_view message) {
  log(LogLevel::WARNING, component, operation, message);
}

void Logger::error(std::string_view component, std::string_view operation, std::string_view message) {
  log(LogLevel::ERROR, component, operation, message);
}

void Logger::critical(std::string_view component, std::string_view operation, std::string_view message) {
  log(LogLevel::CRITICAL, component, operation, message);
}

}  // namespace diagnostics
}  // namespace unilink
