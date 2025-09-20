#pragma once

#include <cstdint>

namespace unilink {
namespace config {

struct TcpServerConfig {
  uint16_t port = 9000;
};

}  // namespace config
}  // namespace unilink