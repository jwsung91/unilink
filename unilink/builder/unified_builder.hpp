#pragma once

#include "unilink/builder/tcp_server_builder.hpp"
#include "unilink/builder/tcp_client_builder.hpp"
#include "unilink/builder/serial_builder.hpp"
#include <cstdint>
#include <string>

namespace unilink {
namespace builder {

/**
 * @brief Unified Builder for creating all types of wrapper instances
 * 
 * Provides a single entry point for creating TcpServer, TcpClient, and Serial
 * wrapper instances using a consistent fluent API pattern.
 */
class UnifiedBuilder {
public:
    /**
     * @brief Create a TcpServer builder
     * @param port The port number for the server
     * @return TcpServerBuilder A configured builder for TcpServer
     */
    static TcpServerBuilder tcp_server(uint16_t port);
    
    /**
     * @brief Create a TcpClient builder
     * @param host The host address to connect to
     * @param port The port number to connect to
     * @return TcpClientBuilder A configured builder for TcpClient
     */
    static TcpClientBuilder tcp_client(const std::string& host, uint16_t port);
    
    /**
     * @brief Create a Serial builder
     * @param device The serial device path (e.g., "/dev/ttyUSB0")
     * @param baud_rate The baud rate for serial communication
     * @return SerialBuilder A configured builder for Serial
     */
    static SerialBuilder serial(const std::string& device, uint32_t baud_rate);
};

} // namespace builder
} // namespace unilink

