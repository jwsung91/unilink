#pragma once

#include <cstddef>

namespace unilink {
namespace common {

// Network and I/O constants
namespace constants {

// Backpressure threshold constants
constexpr size_t DEFAULT_BACKPRESSURE_THRESHOLD = 1 << 20;  // 1 MiB
constexpr size_t DEFAULT_READ_BUFFER_SIZE = 4096;           // 4 KiB

// Retry and timeout constants
constexpr unsigned DEFAULT_RETRY_INTERVAL_MS = 2000;        // 2 seconds
constexpr unsigned DEFAULT_CONNECTION_TIMEOUT_MS = 5000;    // 5 seconds

// Queue management constants
constexpr int DEFAULT_MAX_RETRIES = -1;                     // Unlimited retries

} // namespace constants

} // namespace common
} // namespace unilink
