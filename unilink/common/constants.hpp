#pragma once

#include <cstddef>

namespace unilink {
namespace common {

// Network and I/O constants
namespace constants {

// Backpressure threshold constants
constexpr size_t DEFAULT_BACKPRESSURE_THRESHOLD = 1 << 20;  // 1 MiB
constexpr size_t MIN_BACKPRESSURE_THRESHOLD = 1024;         // 1 KiB minimum
constexpr size_t MAX_BACKPRESSURE_THRESHOLD = 100 << 20;    // 100 MiB maximum
constexpr size_t DEFAULT_READ_BUFFER_SIZE = 4096;           // 4 KiB

// Retry and timeout constants
constexpr unsigned DEFAULT_RETRY_INTERVAL_MS = 2000;      // 2 seconds
constexpr unsigned MIN_RETRY_INTERVAL_MS = 100;           // 100ms minimum
constexpr unsigned MAX_RETRY_INTERVAL_MS = 300000;        // 5 minutes maximum
constexpr unsigned DEFAULT_CONNECTION_TIMEOUT_MS = 5000;  // 5 seconds
constexpr unsigned MIN_CONNECTION_TIMEOUT_MS = 100;       // 100ms minimum
constexpr unsigned MAX_CONNECTION_TIMEOUT_MS = 300000;    // 5 minutes maximum

// Queue management constants
constexpr int DEFAULT_MAX_RETRIES = -1;  // Unlimited retries
constexpr int MAX_RETRIES_LIMIT = 1000;  // Maximum retry limit

// Memory pool constants
constexpr size_t DEFAULT_MEMORY_POOL_SIZE = 100;  // Number of pre-allocated buffers
constexpr size_t MIN_MEMORY_POOL_SIZE = 10;       // Minimum pool size
constexpr size_t MAX_MEMORY_POOL_SIZE = 1000;     // Maximum pool size

// Buffer size constants
constexpr size_t MAX_BUFFER_SIZE = 64 * 1024 * 1024;  // 64MB maximum buffer size
constexpr size_t MIN_BUFFER_SIZE = 1;                 // 1 byte minimum buffer size
constexpr size_t DEFAULT_BUFFER_SIZE = 4096;          // 4KB default buffer size
constexpr size_t LARGE_BUFFER_THRESHOLD = 65536;      // 64KB threshold for large buffers

// Performance and cleanup constants
constexpr unsigned DEFAULT_CLEANUP_INTERVAL_MS = 100;        // 100ms default cleanup interval
constexpr unsigned MIN_CLEANUP_INTERVAL_MS = 10;             // 10ms minimum cleanup interval
constexpr unsigned MAX_CLEANUP_INTERVAL_MS = 1000;           // 1s maximum cleanup interval
constexpr unsigned DEFAULT_HEALTH_CHECK_INTERVAL_MS = 1000;  // 1s health check interval

// Connection and session constants
constexpr size_t DEFAULT_MAX_CONNECTIONS = 1000;      // Default maximum connections
constexpr size_t MAX_MAX_CONNECTIONS = 10000;         // Maximum allowed connections
constexpr size_t DEFAULT_SESSION_TIMEOUT_MS = 30000;  // 30s default session timeout
constexpr size_t MIN_SESSION_TIMEOUT_MS = 1000;       // 1s minimum session timeout
constexpr size_t MAX_SESSION_TIMEOUT_MS = 300000;     // 5m maximum session timeout

// Error handling constants
constexpr size_t DEFAULT_MAX_RECENT_ERRORS = 1000;           // Default max recent errors to track
constexpr size_t MAX_MAX_RECENT_ERRORS = 10000;              // Maximum recent errors to track
constexpr size_t DEFAULT_ERROR_CLEANUP_INTERVAL_MS = 60000;  // 1m error cleanup interval

// Validation constants
constexpr size_t MAX_HOSTNAME_LENGTH = 253;     // Maximum hostname length (RFC 1123)
constexpr size_t MAX_DEVICE_PATH_LENGTH = 256;  // Maximum device path length
constexpr uint32_t MIN_BAUD_RATE = 50;          // Minimum baud rate
constexpr uint32_t MAX_BAUD_RATE = 4000000;     // Maximum baud rate
constexpr uint8_t MIN_DATA_BITS = 5;            // Minimum data bits
constexpr uint8_t MAX_DATA_BITS = 8;            // Maximum data bits
constexpr uint8_t MIN_STOP_BITS = 1;            // Minimum stop bits
constexpr uint8_t MAX_STOP_BITS = 2;            // Maximum stop bits

// Threading and concurrency constants
constexpr size_t DEFAULT_THREAD_POOL_SIZE = 4;               // Default thread pool size
constexpr size_t MIN_THREAD_POOL_SIZE = 1;                   // Minimum thread pool size
constexpr size_t MAX_THREAD_POOL_SIZE = 64;                  // Maximum thread pool size
constexpr unsigned DEFAULT_THREAD_STACK_SIZE = 1024 * 1024;  // 1MB default stack size

}  // namespace constants

}  // namespace common
}  // namespace unilink
