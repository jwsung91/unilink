#include "unilink/builder/unified_builder.hpp"

namespace unilink {
namespace builder {

TcpServerBuilder UnifiedBuilder::tcp_server(uint16_t port) {
    return TcpServerBuilder(port);
}

TcpClientBuilder UnifiedBuilder::tcp_client(const std::string& host, uint16_t port) {
    return TcpClientBuilder(host, port);
}

SerialBuilder UnifiedBuilder::serial(const std::string& device, uint32_t baud_rate) {
    return SerialBuilder(device, baud_rate);
}

} // namespace builder
} // namespace unilink

