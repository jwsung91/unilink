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
constexpr unsigned DEFAULT_RETRY_INTERVAL_MS = 2000;        // 2 seconds
constexpr unsigned MIN_RETRY_INTERVAL_MS = 100;             // 100ms minimum
constexpr unsigned MAX_RETRY_INTERVAL_MS = 60000;           // 60 seconds maximum
constexpr unsigned DEFAULT_CONNECTION_TIMEOUT_MS = 5000;    // 5 seconds

// Queue management constants
constexpr int DEFAULT_MAX_RETRIES = -1;                     // Unlimited retries
constexpr int MAX_RETRIES_LIMIT = 1000;                     // Maximum retry limit

// Memory pool constants
constexpr size_t DEFAULT_MEMORY_POOL_SIZE = 100;            // Number of pre-allocated buffers
constexpr size_t MIN_MEMORY_POOL_SIZE = 10;                 // Minimum pool size
constexpr size_t MAX_MEMORY_POOL_SIZE = 1000;               // Maximum pool size

} // namespace constants

} // namespace common
} // namespace unilink
