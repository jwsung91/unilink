#pragma once

#include <cstdint>
#include <string>

namespace unilink {
namespace config {

struct TcpClientConfig {
  std::string host = "127.0.0.1";
  uint16_t port = 9000;
  unsigned retry_interval_ms = 2000;  // 2s
};

}  // namespace config
}  // namespace unilink