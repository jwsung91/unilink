#pragma once

#include "unilink/builder/tcp_server_builder.hpp"
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/serial_builder.hpp"

namespace unilink {
namespace builder {

// 통합 Builder 클래스 - 모든 통신 타입을 지원
class UnifiedBuilder {
public:
    // TCP 서버 빌더
    static TcpServerBuilder tcp_server(uint16_t port) {
        return TcpServerBuilder(port);
    }
    
    // TCP 클라이언트 빌더
    static TcpClientBuilder tcp_client(const std::string& host, uint16_t port) {
        return TcpClientBuilder(host, port);
    }
    
    // 시리얼 빌더
    static SerialBuilder serial(const std::string& device, uint32_t baud_rate = 9600) {
        return SerialBuilder(device, baud_rate);
    }
};

} // namespace builder
} // namespace unilink
