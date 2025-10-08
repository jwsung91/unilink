#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <vector>
#include <functional>
#include "log_rotation.hpp"

namespace unilink {
namespace common {

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

/**
 * @brief Log output destinations
 */
enum class LogOutput {
    CONSOLE = 0x01,
    FILE = 0x02,
    CALLBACK = 0x04
};

/**
 * @brief Centralized logging system
 * 
 * Provides thread-safe, configurable logging with multiple output destinations
 * and performance optimizations for production use.
 */
class Logger {
public:
    using LogCallback = std::function<void(LogLevel level, const std::string& formatted_message)>;
    
    /**
     * @brief Get singleton instance
     */
    static Logger& instance();
    
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
    void log(LogLevel level, const std::string& component, 
             const std::string& operation, const std::string& message);
    
    void debug(const std::string& component, const std::string& operation, 
               const std::string& message);
    void info(const std::string& component, const std::string& operation, 
              const std::string& message);
    void warning(const std::string& component, const std::string& operation, 
                 const std::string& message);
    void error(const std::string& component, const std::string& operation, 
               const std::string& message);
    void critical(const std::string& component, const std::string& operation, 
                  const std::string& message);

private:
    Logger();
    ~Logger();
    
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
    
    std::string format_message(LogLevel level, const std::string& component,
                              const std::string& operation, const std::string& message);
    std::string level_to_string(LogLevel level);
    std::string get_timestamp();
    void write_to_console(const std::string& message);
    void write_to_file(const std::string& message);
    void call_callback(LogLevel level, const std::string& message);
    
    // Log rotation helper methods
    void check_and_rotate_log();
    void open_log_file(const std::string& filename);
};

/**
 * @brief Convenience macros for logging
 */
#define UNILINK_LOG_DEBUG(component, operation, message) \
    unilink::common::Logger::instance().debug(component, operation, message)

#define UNILINK_LOG_INFO(component, operation, message) \
    unilink::common::Logger::instance().info(component, operation, message)

#define UNILINK_LOG_WARNING(component, operation, message) \
    unilink::common::Logger::instance().warning(component, operation, message)

#define UNILINK_LOG_ERROR(component, operation, message) \
    unilink::common::Logger::instance().error(component, operation, message)

#define UNILINK_LOG_CRITICAL(component, operation, message) \
    unilink::common::Logger::instance().critical(component, operation, message)

/**
 * @brief Conditional logging macros (only evaluate message if level is enabled)
 */
#define UNILINK_LOG_DEBUG_IF(component, operation, message) \
    do { \
        if (unilink::common::Logger::instance().get_level() <= unilink::common::LogLevel::DEBUG) { \
            UNILINK_LOG_DEBUG(component, operation, message); \
        } \
    } while(0)

#define UNILINK_LOG_INFO_IF(component, operation, message) \
    do { \
        if (unilink::common::Logger::instance().get_level() <= unilink::common::LogLevel::INFO) { \
            UNILINK_LOG_INFO(component, operation, message); \
        } \
    } while(0)

/**
 * @brief Performance logging macros for expensive operations
 */
#define UNILINK_LOG_PERF_START(component, operation) \
    auto _perf_start_##operation = std::chrono::high_resolution_clock::now()

#define UNILINK_LOG_PERF_END(component, operation) \
    do { \
        auto _perf_end_##operation = std::chrono::high_resolution_clock::now(); \
        auto _perf_duration_##operation = std::chrono::duration_cast<std::chrono::microseconds>( \
            _perf_end_##operation - _perf_start_##operation).count(); \
        UNILINK_LOG_DEBUG(component, operation, "Duration: " + std::to_string(_perf_duration_##operation) + " μs"); \
    } while(0)

} // namespace common
} // namespace unilink
