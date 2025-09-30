#pragma once

#include <cstdint>
#include <string>
#include "unilink/common/constants.hpp"

namespace unilink {
namespace config {

struct TcpClientConfig {
  std::string host = "127.0.0.1";
  uint16_t port = 9000;
  unsigned retry_interval_ms = common::constants::DEFAULT_RETRY_INTERVAL_MS;
  unsigned connection_timeout_ms = common::constants::DEFAULT_CONNECTION_TIMEOUT_MS;
  int max_retries = common::constants::DEFAULT_MAX_RETRIES;
  size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  bool enable_memory_pool = true;
  
  // Validation methods
  bool is_valid() const {
    return !host.empty() && 
           port > 0 && 
           retry_interval_ms >= common::constants::MIN_RETRY_INTERVAL_MS &&
           retry_interval_ms <= common::constants::MAX_RETRY_INTERVAL_MS &&
           backpressure_threshold >= common::constants::MIN_BACKPRESSURE_THRESHOLD &&
           backpressure_threshold <= common::constants::MAX_BACKPRESSURE_THRESHOLD &&
           (max_retries == -1 || (max_retries >= 0 && max_retries <= common::constants::MAX_RETRIES_LIMIT));
  }
  
  
  // Apply validation and clamp values to valid ranges
  void validate_and_clamp() {
    if (retry_interval_ms < common::constants::MIN_RETRY_INTERVAL_MS) {
      retry_interval_ms = common::constants::MIN_RETRY_INTERVAL_MS;
    } else if (retry_interval_ms > common::constants::MAX_RETRY_INTERVAL_MS) {
      retry_interval_ms = common::constants::MAX_RETRY_INTERVAL_MS;
    }
    
    if (backpressure_threshold < common::constants::MIN_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;
    } else if (backpressure_threshold > common::constants::MAX_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MAX_BACKPRESSURE_THRESHOLD;
    }
    
    if (max_retries != -1 && max_retries > common::constants::MAX_RETRIES_LIMIT) {
      max_retries = common::constants::MAX_RETRIES_LIMIT;
    }
  }
};

}  // namespace config
}  // namespace unilink