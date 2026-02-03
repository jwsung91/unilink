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
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <thread>

namespace unilink {
namespace diagnostics {

Logger::Logger() = default;

Logger::~Logger() {
  teardown_async_logging();
  flush();
}

Logger& Logger::default_logger() {
  static Logger instance;
  return instance;
}

Logger& Logger::instance() { return default_logger(); }

void Logger::set_level(LogLevel level) { current_level_.store(level); }

LogLevel Logger::get_level() const { return current_level_.load(); }

void Logger::set_console_output(bool enable) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (enable) {
    outputs_.fetch_or(static_cast<int>(LogOutput::CONSOLE));
  } else {
    outputs_.fetch_and(~static_cast<int>(LogOutput::CONSOLE));
  }
}

void Logger::set_file_output(const std::string& filename) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (filename.empty()) {
    file_output_.reset();
    log_rotation_.reset();
    current_log_file_.clear();
    outputs_.fetch_and(~static_cast<int>(LogOutput::FILE));
  } else {
    open_log_file(filename);
  }
}

void Logger::set_file_output_with_rotation(const std::string& filename, const LogRotationConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (filename.empty()) {
    file_output_.reset();
    log_rotation_.reset();
    current_log_file_.clear();
    outputs_.fetch_and(~static_cast<int>(LogOutput::FILE));
  } else {
    log_rotation_ = std::make_unique<LogRotation>(config);
    current_log_file_ = filename;
    open_log_file(filename);
  }
}

void Logger::set_callback(LogCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  callback_ = std::move(callback);
  if (callback_) {
    outputs_.fetch_or(static_cast<int>(LogOutput::CALLBACK));
  } else {
    outputs_.fetch_and(~static_cast<int>(LogOutput::CALLBACK));
  }
}

void Logger::set_outputs(int outputs) { outputs_.store(outputs); }

void Logger::set_enabled(bool enabled) { enabled_.store(enabled); }

bool Logger::is_enabled() const { return enabled_.load(); }

void Logger::set_format(const std::string& format) {
  std::lock_guard<std::mutex> lock(mutex_);
  format_string_ = format;
}

void Logger::flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_output_ && file_output_->is_open()) {
    file_output_->flush();
  }
  std::cout.flush();
  std::cerr.flush();
}

void Logger::log(LogLevel level, const std::string& component, const std::string& operation,
                 const std::string& message) {
  if (!enabled_.load() || level < current_level_.load()) {
    return;
  }

  // Use async logging if enabled
  if (async_enabled_.load()) {
    LogEntry entry(level, component, operation, message);

    // Always update total logs count
    update_stats_on_enqueue();

    // Check backpressure
    if (async_config_.enable_backpressure && should_drop_log()) {
      update_stats_on_drop();
      return;
    }

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      log_queue_.push(entry);
    }

    queue_cv_.notify_one();
    return;  // Successfully queued
  }

  // Synchronous logging (when async is disabled)
  std::string formatted_message = format_message(level, component, operation, message);
  int current_outputs = outputs_.load();

  if (current_outputs & static_cast<int>(LogOutput::CONSOLE)) {
    write_to_console(formatted_message);
  }

  if (current_outputs & static_cast<int>(LogOutput::FILE)) {
    check_and_rotate_log();
    write_to_file(formatted_message);
  }

  if (current_outputs & static_cast<int>(LogOutput::CALLBACK)) {
    call_callback(level, formatted_message);
  }
}

void Logger::debug(const std::string& component, const std::string& operation, const std::string& message) {
  log(LogLevel::DEBUG, component, operation, message);
}

void Logger::info(const std::string& component, const std::string& operation, const std::string& message) {
  log(LogLevel::INFO, component, operation, message);
}

void Logger::warning(const std::string& component, const std::string& operation, const std::string& message) {
  log(LogLevel::WARNING, component, operation, message);
}

void Logger::error(const std::string& component, const std::string& operation, const std::string& message) {
  log(LogLevel::ERROR, component, operation, message);
}

void Logger::critical(const std::string& component, const std::string& operation, const std::string& message) {
  log(LogLevel::CRITICAL, component, operation, message);
}

std::string Logger::format_message(LogLevel level, const std::string& component, const std::string& operation,
                                   const std::string& message) {
  std::string result = format_string_;

  // Replace placeholders
  std::string timestamp = get_timestamp();
  std::string level_str = level_to_string(level);

  // Simple string replacement
  size_t pos = 0;
  while ((pos = result.find("{timestamp}", pos)) != std::string::npos) {
    result.replace(pos, 11, timestamp);
    pos += timestamp.length();
  }

  pos = 0;
  while ((pos = result.find("{level}", pos)) != std::string::npos) {
    result.replace(pos, 7, level_str);
    pos += level_str.length();
  }

  pos = 0;
  while ((pos = result.find("{component}", pos)) != std::string::npos) {
    result.replace(pos, 11, component);
    pos += component.length();
  }

  pos = 0;
  while ((pos = result.find("{operation}", pos)) != std::string::npos) {
    result.replace(pos, 11, operation);
    pos += operation.length();
  }

  pos = 0;
  while ((pos = result.find("{message}", pos)) != std::string::npos) {
    result.replace(pos, 9, message);
    pos += message.length();
  }

  return result;
}

std::string Logger::level_to_string(LogLevel level) {
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

std::string Logger::get_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::tm time_info{};

#if defined(_WIN32)
  ::localtime_s(&time_info, &time_t);
#else
  ::localtime_r(&time_t, &time_info);
#endif

  std::ostringstream oss;
  oss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return oss.str();
}

void Logger::write_to_console(const std::string& message) {
  // Use stderr for ERROR and CRITICAL levels
  if (message.find("[ERROR]") != std::string::npos || message.find("[CRITICAL]") != std::string::npos) {
    std::cerr << message << '\n';
  } else {
    std::cout << message << '\n';
  }
}

void Logger::write_to_file(const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_output_ && file_output_->is_open()) {
    *file_output_ << message << std::endl;
  }
}

void Logger::call_callback(LogLevel level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (callback_) {
    try {
      callback_(level, message);
    } catch (const std::exception& e) {
      // Avoid infinite recursion - log to stderr instead
      std::cerr << "Error in log callback: " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Unknown error in log callback" << std::endl;
    }
  }
}

void Logger::check_and_rotate_log() {
  if (!log_rotation_ || current_log_file_.empty()) {
    return;
  }

  // Ensure single-threaded access to file handle while rotating
  std::lock_guard<std::mutex> lock(mutex_);

  if (!log_rotation_->should_rotate(current_log_file_)) {
    return;
  }

  // Flush and close the current file handle so Windows can rename it
  if (file_output_) {
    file_output_->flush();
    file_output_->close();
    file_output_.reset();
  }

  std::string new_filepath = log_rotation_->rotate(current_log_file_);

  // Always attempt to reopen the active log file, even if rotation failed
  open_log_file(current_log_file_);

  // If rotation succeeded, new_filepath will differ, but no additional action needed
}

void Logger::open_log_file(const std::string& filename) {
  file_output_ = std::make_unique<std::ofstream>(filename, std::ios::app);
  if (file_output_->is_open()) {
    outputs_.fetch_or(static_cast<int>(LogOutput::FILE));
  } else {
    file_output_.reset();
    std::cerr << "Failed to open log file: " << filename << std::endl;
  }
}

void Logger::set_async_logging(bool enable, const AsyncLogConfig& config) {
  if (enable) {
    setup_async_logging(config);
  } else {
    teardown_async_logging();
  }
}

bool Logger::is_async_logging_enabled() const { return async_enabled_.load(); }

AsyncLogStats Logger::get_async_stats() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  AsyncLogStats result = async_stats_;
  result.queue_size = get_queue_size();
  result.max_queue_size_reached = std::max(result.max_queue_size_reached, result.queue_size);
  return result;
}

void Logger::setup_async_logging(const AsyncLogConfig& config) {
  teardown_async_logging();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    async_config_ = config;
    async_stats_.reset();
    shutdown_requested_.store(false);
    running_.store(true);
    async_enabled_.store(true);
  }

  worker_thread_ = std::thread(&Logger::worker_loop, this);
}

void Logger::teardown_async_logging() {
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

void Logger::worker_loop() {
  std::vector<LogEntry> batch;
  batch.reserve(async_config_.batch_size);

  auto last_flush = std::chrono::steady_clock::now();

  while (true) {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Wait for logs or timeout
    bool has_logs = queue_cv_.wait_for(lock, async_config_.flush_interval,
                                       [this] { return !log_queue_.empty() || shutdown_requested_.load(); });

    // Collect logs for batch processing
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

      // Process batch
      if (!batch.empty()) {
        process_batch(batch);
        update_stats_on_batch(batch.size());
      }
    } else {
      lock.unlock();
    }

    // Periodic flush
    auto now = std::chrono::steady_clock::now();
    if (now - last_flush >= async_config_.flush_interval) {
      last_flush = now;
    }

    if (shutdown_requested_.load() && log_queue_.empty()) {
      break;
    }
  }

  flush();
  running_.store(false);
}

void Logger::process_batch(const std::vector<LogEntry>& batch) {
  for (const auto& entry : batch) {
    std::string formatted_message = format_message(entry.level, entry.component, entry.operation, entry.message);
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

bool Logger::should_drop_log() const {
  size_t current_size = get_queue_size();
  return current_size >= async_config_.max_queue_size;
}

size_t Logger::get_queue_size() const {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  return log_queue_.size();
}

void Logger::clear_queue() {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  std::queue<LogEntry> empty;
  log_queue_.swap(empty);
}

void Logger::update_stats_on_enqueue() {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  async_stats_.total_logs++;
}

void Logger::update_stats_on_drop() {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  async_stats_.dropped_logs++;
}

void Logger::update_stats_on_batch(size_t /* batch_size */) {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  async_stats_.batch_count++;
}

void Logger::update_stats_on_flush() {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  async_stats_.flush_count++;
}

}  // namespace diagnostics
}  // namespace unilink
