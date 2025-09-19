#pragma once

#include <string>

struct TcpClientConfig {
  std::string host = "127.0.0.1";
  uint16_t port = 9000;
  unsigned retry_interval_ms = 2000;  // 2s
};
