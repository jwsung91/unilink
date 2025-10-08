#pragma once

#include <boost/system/error_code.hpp>
#include <string>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace unilink {
namespace common {

/**
 * @brief Error severity levels
 */
enum class ErrorLevel {
    INFO = 0,      // 정보성 메시지 (정상 동작 정보)
    WARNING = 1,   // 경고 (복구 가능한 문제)
    ERROR = 2,     // 에러 (복구 시도 필요)
    CRITICAL = 3   // 치명적 에러 (복구 불가)
};

/**
 * @brief Error categories for classification
 */
enum class ErrorCategory {
    CONNECTION = 0,    // 연결 관련 (TCP, Serial 연결/해제)
    COMMUNICATION = 1, // 통신 관련 (데이터 송수신)
    CONFIGURATION = 2, // 설정 관련 (잘못된 설정값)
    MEMORY = 3,        // 메모리 관련 (할당/해제 오류)
    SYSTEM = 4,        // 시스템 관련 (OS 레벨 오류)
    UNKNOWN = 5        // 알 수 없는 오류
};

/**
 * @brief Comprehensive error information structure
 */
struct ErrorInfo {
    ErrorLevel level;                              // 에러 심각도
    ErrorCategory category;                        // 에러 카테고리
    std::string component;                         // 컴포넌트 이름 (serial, tcp_server, tcp_client)
    std::string operation;                         // 수행 중이던 작업 (read, write, connect, bind)
    std::string message;                           // 에러 메시지
    boost::system::error_code boost_error;         // Boost 에러 코드
    std::chrono::system_clock::time_point timestamp; // 에러 발생 시간
    bool retryable;                                // 재시도 가능 여부
    uint32_t retry_count;                          // 현재 재시도 횟수
    std::string context;                           // 추가 컨텍스트 정보
    
    /**
     * @brief Constructor for basic error info
     */
    ErrorInfo(ErrorLevel l, ErrorCategory c, const std::string& comp, 
              const std::string& op, const std::string& msg)
        : level(l), category(c), component(comp), operation(op), message(msg),
          timestamp(std::chrono::system_clock::now()), retryable(false), retry_count(0) {}
    
    /**
     * @brief Constructor with Boost error code
     */
    ErrorInfo(ErrorLevel l, ErrorCategory c, const std::string& comp, 
              const std::string& op, const std::string& msg, 
              const boost::system::error_code& ec, bool retry = false)
        : level(l), category(c), component(comp), operation(op), message(msg),
          boost_error(ec), timestamp(std::chrono::system_clock::now()), 
          retryable(retry), retry_count(0) {}
    
    /**
     * @brief Get formatted timestamp string
     */
    std::string get_timestamp_string() const {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
    
    /**
     * @brief Get error level as string
     */
    std::string get_level_string() const {
        switch (level) {
            case ErrorLevel::INFO: return "INFO";
            case ErrorLevel::WARNING: return "WARNING";
            case ErrorLevel::ERROR: return "ERROR";
            case ErrorLevel::CRITICAL: return "CRITICAL";
        }
        return "UNKNOWN";
    }
    
    /**
     * @brief Get error category as string
     */
    std::string get_category_string() const {
        switch (category) {
            case ErrorCategory::CONNECTION: return "CONNECTION";
            case ErrorCategory::COMMUNICATION: return "COMMUNICATION";
            case ErrorCategory::CONFIGURATION: return "CONFIGURATION";
            case ErrorCategory::MEMORY: return "MEMORY";
            case ErrorCategory::SYSTEM: return "SYSTEM";
            case ErrorCategory::UNKNOWN: return "UNKNOWN";
        }
        return "UNKNOWN";
    }
    
    /**
     * @brief Get formatted error summary
     */
    std::string get_summary() const {
        std::ostringstream oss;
        oss << "[" << get_level_string() << "] " 
            << "[" << component << "] " 
            << "[" << operation << "] " 
            << message;
        
        if (boost_error) {
            oss << " (boost: " << boost_error.message() 
                << ", code: " << boost_error.value() << ")";
        }
        
        if (retryable) {
            oss << " [RETRYABLE, count: " << retry_count << "]";
        }
        
        return oss.str();
    }
};

/**
 * @brief Error statistics for monitoring
 */
struct ErrorStats {
    size_t total_errors = 0;
    size_t errors_by_level[4] = {0, 0, 0, 0};     // INFO, WARNING, ERROR, CRITICAL
    size_t errors_by_category[6] = {0, 0, 0, 0, 0, 0}; // CONNECTION, COMMUNICATION, etc.
    size_t retryable_errors = 0;
    size_t successful_retries = 0;
    size_t failed_retries = 0;
    
    std::chrono::system_clock::time_point first_error;
    std::chrono::system_clock::time_point last_error;
    
    /**
     * @brief Reset all statistics
     */
    void reset() {
        total_errors = 0;
        std::fill(std::begin(errors_by_level), std::end(errors_by_level), 0);
        std::fill(std::begin(errors_by_category), std::end(errors_by_category), 0);
        retryable_errors = 0;
        successful_retries = 0;
        failed_retries = 0;
        first_error = std::chrono::system_clock::time_point{};
        last_error = std::chrono::system_clock::time_point{};
    }
    
    /**
     * @brief Get error rate (errors per minute)
     */
    double get_error_rate() const {
        if (total_errors == 0) return 0.0;
        
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(
            last_error - first_error).count();
        return duration > 0 ? static_cast<double>(total_errors) / duration : 0.0;
    }
};

} // namespace common
} // namespace unilink
