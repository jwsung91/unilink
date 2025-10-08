#pragma once

#include <cstdint>
#include "unilink/common/constants.hpp"

namespace unilink {
namespace config {

struct TcpServerConfig {
  uint16_t port = 9000;
  size_t backpressure_threshold = common::constants::DEFAULT_BACKPRESSURE_THRESHOLD;
  bool enable_memory_pool = true;
  int max_connections = 100;  // Maximum concurrent connections
  
  // Port binding retry configuration
  bool enable_port_retry = false;  // Enable port binding retry
  int max_port_retries = 3;        // Maximum number of retry attempts
  int port_retry_interval_ms = 1000;  // Retry interval in milliseconds
  
  // Validation methods
  bool is_valid() const {
    return port > 0 && 
           backpressure_threshold >= common::constants::MIN_BACKPRESSURE_THRESHOLD &&
           backpressure_threshold <= common::constants::MAX_BACKPRESSURE_THRESHOLD &&
           max_connections > 0;
  }
  
  // Apply validation and clamp values to valid ranges
  void validate_and_clamp() {
    if (backpressure_threshold < common::constants::MIN_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;
    } else if (backpressure_threshold > common::constants::MAX_BACKPRESSURE_THRESHOLD) {
      backpressure_threshold = common::constants::MAX_BACKPRESSURE_THRESHOLD;
    }
    
    if (max_connections <= 0) {
      max_connections = 1;
    }
  }
};

}  // namespace config
}  // namespace unilink