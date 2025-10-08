#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>

namespace unilink {
namespace common {

Logger::Logger() = default;

Logger::~Logger() {
    flush();
}

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(LogLevel level) {
    current_level_.store(level);
}

LogLevel Logger::get_level() const {
    return current_level_.load();
}

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

void Logger::set_file_output_with_rotation(const std::string& filename, 
                                          const LogRotationConfig& config) {
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

void Logger::set_outputs(int outputs) {
    outputs_.store(outputs);
}

void Logger::set_enabled(bool enabled) {
    enabled_.store(enabled);
}

bool Logger::is_enabled() const {
    return enabled_.load();
}

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

void Logger::log(LogLevel level, const std::string& component, 
                 const std::string& operation, const std::string& message) {
    if (!enabled_.load() || level < current_level_.load()) {
        return;
    }
    
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

void Logger::debug(const std::string& component, const std::string& operation, 
                   const std::string& message) {
    log(LogLevel::DEBUG, component, operation, message);
}

void Logger::info(const std::string& component, const std::string& operation, 
                  const std::string& message) {
    log(LogLevel::INFO, component, operation, message);
}

void Logger::warning(const std::string& component, const std::string& operation, 
                     const std::string& message) {
    log(LogLevel::WARNING, component, operation, message);
}

void Logger::error(const std::string& component, const std::string& operation, 
                   const std::string& message) {
    log(LogLevel::ERROR, component, operation, message);
}

void Logger::critical(const std::string& component, const std::string& operation, 
                      const std::string& message) {
    log(LogLevel::CRITICAL, component, operation, message);
}

std::string Logger::format_message(LogLevel level, const std::string& component,
                                  const std::string& operation, const std::string& message) {
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
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Logger::write_to_console(const std::string& message) {
    // Use stderr for ERROR and CRITICAL levels
    if (message.find("[ERROR]") != std::string::npos || 
        message.find("[CRITICAL]") != std::string::npos) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
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
    
    if (log_rotation_->should_rotate(current_log_file_)) {
        std::string new_filepath = log_rotation_->rotate(current_log_file_);
        if (new_filepath != current_log_file_) {
            // File was rotated, reopen the original file
            open_log_file(current_log_file_);
        }
    }
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

} // namespace common
} // namespace unilink
