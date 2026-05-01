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

#include <spdlog/async.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <future>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>

namespace unilink {
namespace diagnostics {

/**
 * @brief Custom spdlog sink for LogCallback
 */
template <typename Mutex>
class callback_sink : public spdlog::sinks::base_sink<Mutex> {
 public:
  explicit callback_sink(Logger::LogCallback callback) : callback_(std::move(callback)) {}

  void set_callback(Logger::LogCallback callback) {
    std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
    callback_ = std::move(callback);
  }

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    if (!callback_) return;

    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    callback_(from_spdlog_level(msg.level), fmt::to_string(formatted));
  }

  void flush_() override {}

 private:
  static LogLevel from_spdlog_level(spdlog::level::level_enum level) {
    switch (level) {
      case spdlog::level::debug:
        return LogLevel::DEBUG;
      case spdlog::level::info:
        return LogLevel::INFO;
      case spdlog::level::warn:
        return LogLevel::WARNING;
      case spdlog::level::err:
        return LogLevel::ERROR;
      case spdlog::level::critical:
        return LogLevel::CRITICAL;
      default:
        return LogLevel::INFO;
    }
  }

  Logger::LogCallback callback_;
};

using callback_sink_mt = callback_sink<std::mutex>;

struct Logger::Impl {
  mutable std::mutex mutex_;
  std::atomic<LogLevel> current_level_{LogLevel::INFO};
  std::atomic<bool> enabled_{true};
  std::atomic<int> outputs_{static_cast<int>(LogOutput::CONSOLE)};

  std::shared_ptr<spdlog::logger> spd_logger_;
  std::shared_ptr<spdlog::sinks::dist_sink_mt> dist_sink_;
  std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
  std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> file_sink_;
  std::shared_ptr<callback_sink_mt> callback_sink_;

  std::string current_log_file_;
  LogRotationConfig rotation_config_;

  // Async logging support
  std::atomic<bool> async_enabled_{false};
  AsyncLogConfig async_config_;

  Impl() {
    dist_sink_ = std::make_shared<spdlog::sinks::dist_sink_mt>();

    // Default to sync logger initially, can be changed to async via set_async_logging
    spd_logger_ = std::make_shared<spdlog::logger>("unilink", dist_sink_);
    spd_logger_->set_level(to_spdlog_level(LogLevel::DEBUG));  // Allow all, filter via Logger::log or Logger::set_level

    // Initial setup with console sink
    console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    dist_sink_->add_sink(console_sink_);

    set_format("{timestamp} [{level}] [{component}] [{operation}] {message}");
  }

  ~Impl() { spdlog::drop("unilink"); }

  void parse_format(const std::string& format) {
    std::string pattern = format;

    // Map unilink placeholders to spdlog pattern
    // {timestamp} -> %Y-%m-%d %H:%M:%S.%e
    // {level}     -> %l
    // {message}   -> %v (Note: macros wrap component/operation into message)

    auto replace_all = [](std::string& str, const std::string& from, const std::string& to) {
      size_t pos = 0;
      while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
      }
    };

    replace_all(pattern, "{timestamp}", "%Y-%m-%d %H:%M:%S.%e");
    replace_all(pattern, "{level}", "%l");
    replace_all(pattern, "{message}", "%v");

    // component and operation are currently embedded in the message by macros
    // but if they appear in format, we strip them or handle as literal since they are in %v
    replace_all(pattern, "[{component}] ", "");
    replace_all(pattern, "[{operation}] ", "");

    dist_sink_->set_pattern(pattern);
  }

  void set_format(const std::string& format) {
    parse_format(format);
  }

  void flush() {
    if (spd_logger_) {
      spd_logger_->flush();
    }
    if (dist_sink_) {
      dist_sink_->flush();
    }
  }

  void setup_async_logging(const AsyncLogConfig& config) {
    async_config_ = config;

    spdlog::drop("unilink"); // Drop existing logger

    auto overflow_policy = config.enable_backpressure ? 
                           spdlog::async_overflow_policy::block : 
                           spdlog::async_overflow_policy::overrun_oldest;

    if (!spdlog::thread_pool()) {
        spdlog::init_thread_pool(config.max_queue_size, 1);
    }

    spd_logger_ = std::make_shared<spdlog::async_logger>(
        "unilink", dist_sink_, spdlog::thread_pool(), overflow_policy);
    spd_logger_->set_level(to_spdlog_level(current_level_.load()));

    spdlog::register_logger(spd_logger_);

    if (config.flush_interval.count() > 0) {
      // spdlog::flush_every takes std::chrono::seconds. 
      // Ensure at least 1 second if a positive interval is requested.
      auto secs = std::max(std::chrono::seconds(1),
                           std::chrono::duration_cast<std::chrono::seconds>(config.flush_interval));
      spdlog::flush_every(secs);
    }

    async_enabled_.store(true);
  }

  void teardown_async_logging() {
    if (!async_enabled_.load()) return;

    async_enabled_.store(false);

    // Use std::async to drop the logger with a timeout
    auto drop_future = std::async(std::launch::async, []() {
      spdlog::drop("unilink");
    });

    if (drop_future.wait_for(async_config_.shutdown_timeout) == std::future_status::timeout) {
      // If we timeout, we can't force spdlog to stop, but we stop waiting.
      // The background thread will eventually finish.
    }

    spd_logger_.reset();
    dist_sink_->flush();

    spd_logger_ = std::make_shared<spdlog::logger>("unilink", dist_sink_);
    spd_logger_->set_level(to_spdlog_level(current_level_.load()));
    spdlog::register_logger(spd_logger_);
  }

  static spdlog::level::level_enum to_spdlog_level(LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG:
        return spdlog::level::debug;
      case LogLevel::INFO:
        return spdlog::level::info;
      case LogLevel::WARNING:
        return spdlog::level::warn;
      case LogLevel::ERROR:
        return spdlog::level::err;
      case LogLevel::CRITICAL:
        return spdlog::level::critical;
      default:
        return spdlog::level::info;
    }
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

void Logger::set_level(LogLevel level) {
  impl_->current_level_.store(level);
  impl_->spd_logger_->set_level(Impl::to_spdlog_level(level));
}

LogLevel Logger::level() const { return get_impl()->current_level_.load(); }

void Logger::set_console_output(bool enable) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  if (enable) {
    if (!impl_->console_sink_) {
      impl_->console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      impl_->dist_sink_->add_sink(impl_->console_sink_);
    }
    impl_->outputs_.fetch_or(static_cast<int>(LogOutput::CONSOLE));
  } else {
    if (impl_->console_sink_) {
      impl_->dist_sink_->remove_sink(impl_->console_sink_);
      impl_->console_sink_.reset();
    }
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::CONSOLE));
  }
}

void Logger::set_file_output(const std::string& filename) {
  set_file_output_with_rotation(filename);
}

void Logger::set_file_output_with_rotation(const std::string& filename, const LogRotationConfig& config) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);

  if (filename.empty()) {
    if (impl_->file_sink_) {
      impl_->dist_sink_->remove_sink(impl_->file_sink_);
      impl_->file_sink_.reset();
    }
    impl_->current_log_file_.clear();
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::FILE));
  } else {
    try {
      if (impl_->file_sink_) {
        impl_->dist_sink_->remove_sink(impl_->file_sink_);
      }

      impl_->rotation_config_ = config;
      impl_->current_log_file_ = filename;

      impl_->file_sink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          filename, config.max_file_size_bytes, config.max_files);

      impl_->dist_sink_->add_sink(impl_->file_sink_);
      impl_->outputs_.fetch_or(static_cast<int>(LogOutput::FILE));
    } catch (const spdlog::spdlog_ex& e) {
      std::cerr << "Failed to open log file: " << filename << " (" << e.what() << ")" << std::endl;
      impl_->file_sink_.reset();
      impl_->current_log_file_.clear();
      impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::FILE));
    }
  }
}

void Logger::set_async_logging(bool enable, const AsyncLogConfig& config) {
  if (enable) {
    impl_->setup_async_logging(config);
  } else {
    impl_->teardown_async_logging();
  }
}

bool Logger::async_logging_enabled() const { return get_impl()->async_enabled_.load(); }

void Logger::set_callback(LogCallback callback) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);

  if (!callback) {
    if (impl_->callback_sink_) {
      impl_->dist_sink_->remove_sink(impl_->callback_sink_);
      impl_->callback_sink_.reset();
    }
    impl_->outputs_.fetch_and(~static_cast<int>(LogOutput::CALLBACK));
    return;
  }

  if (impl_->callback_sink_) {
    impl_->callback_sink_->set_callback(std::move(callback));
    // Ensure it's in the dist_sink if outputs bit is set
    if (impl_->outputs_.load() & static_cast<int>(LogOutput::CALLBACK)) {
        impl_->dist_sink_->remove_sink(impl_->callback_sink_); // Avoid duplicates
        impl_->dist_sink_->add_sink(impl_->callback_sink_);
    }
  } else {
    impl_->callback_sink_ = std::make_shared<callback_sink_mt>(std::move(callback));
    impl_->dist_sink_->add_sink(impl_->callback_sink_);
  }
  impl_->outputs_.fetch_or(static_cast<int>(LogOutput::CALLBACK));
}

void Logger::set_outputs(int outputs) {
  std::lock_guard<std::mutex> lock(impl_->mutex_);
  impl_->outputs_.store(outputs);

  // Reconcile sinks in dist_sink_ based on the new bitmask

  // Console Sink
  if (outputs & static_cast<int>(LogOutput::CONSOLE)) {
    if (!impl_->console_sink_) {
      impl_->console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    }
    impl_->dist_sink_->remove_sink(impl_->console_sink_); // Ensure no duplicate
    impl_->dist_sink_->add_sink(impl_->console_sink_);
  } else {
    if (impl_->console_sink_) {
      impl_->dist_sink_->remove_sink(impl_->console_sink_);
      impl_->console_sink_.reset();
    }
  }

  // File Sink
  if (outputs & static_cast<int>(LogOutput::FILE)) {
    if (!impl_->file_sink_ && !impl_->current_log_file_.empty()) {
      impl_->file_sink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          impl_->current_log_file_, impl_->rotation_config_.max_file_size_bytes, impl_->rotation_config_.max_files);
    }
    if (impl_->file_sink_) {
        impl_->dist_sink_->remove_sink(impl_->file_sink_); // Ensure no duplicate
        impl_->dist_sink_->add_sink(impl_->file_sink_);
    }
  } else {
    if (impl_->file_sink_) {
      impl_->dist_sink_->remove_sink(impl_->file_sink_);
      impl_->file_sink_.reset();
    }
  }

  // Callback Sink
  if (outputs & static_cast<int>(LogOutput::CALLBACK)) {
    if (impl_->callback_sink_) {
        impl_->dist_sink_->remove_sink(impl_->callback_sink_); // Ensure no duplicate
        impl_->dist_sink_->add_sink(impl_->callback_sink_);
    }
  } else {
    if (impl_->callback_sink_) {
      impl_->dist_sink_->remove_sink(impl_->callback_sink_);
    }
  }
}

void Logger::set_enabled(bool enabled) { impl_->enabled_.store(enabled); }

bool Logger::enabled() const { return get_impl()->enabled_.load(); }

void Logger::set_format(const std::string& format) { impl_->parse_format(format); }

void Logger::flush() { impl_->flush(); }

void Logger::log(LogLevel level, std::string_view component, std::string_view operation, std::string_view message) {
  if (!get_impl()->enabled_.load() || level < get_impl()->current_level_.load()) {
    return;
  }

  // spdlog handles async internally if spd_logger_ is an async_logger.
  // We'll format the message payload to maintain structured logging look.
  std::string payload;
  payload.reserve(component.length() + operation.length() + message.length() + 8);
  payload.append("[");
  payload.append(component);
  payload.append("] [");
  payload.append(operation);
  payload.append("] ");
  payload.append(message);

  impl_->spd_logger_->log(Impl::to_spdlog_level(level), payload);
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
