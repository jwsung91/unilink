#include <iostream>
#include <thread>
#include <chrono>
#include "unilink/unilink.hpp"

using namespace unilink;

int main() {
    std::cout << "=== Unilink Logging System Usage Example ===" << std::endl;
    
    // 1. Logging system setup
    std::cout << "\n1. Logging system setup" << std::endl;
    
    // Set log level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
    common::Logger::instance().set_level(common::LogLevel::DEBUG);
    
    // Set file output
    common::Logger::instance().set_file_output("unilink_example.log");
    
    // Enable console output
    common::Logger::instance().set_console_output(true);
    
    // 2. Basic logging usage
    std::cout << "\n2. Basic logging usage" << std::endl;
    
    UNILINK_LOG_INFO("example", "startup", "Application started");
    UNILINK_LOG_DEBUG("example", "config", "Loading configuration...");
    UNILINK_LOG_WARNING("example", "config", "Some settings using default values");
    
    // 3. TCP server creation and logging
    std::cout << "\n3. TCP server creation and logging" << std::endl;
    
    auto server = tcp_server(8080)
        .auto_start(true)
        .on_connect([]() {
            UNILINK_LOG_INFO("tcp_server", "connect", "Client connected");
        })
        .on_data([](const std::string& data) {
            UNILINK_LOG_DEBUG("tcp_server", "data", "Data received: " + data);
        })
        .on_error([](const std::string& error) {
            UNILINK_LOG_ERROR("tcp_server", "error", "Server error: " + error);
        })
        .build();
    
    if (server) {
        UNILINK_LOG_INFO("example", "server", "TCP server started on port 8080");
    }
    
    // 4. TCP client creation and logging
    std::cout << "\n4. TCP client creation and logging" << std::endl;
    
    auto client = tcp_client("127.0.0.1", 8080)
        .auto_start(true)
        .on_connect([]() {
            UNILINK_LOG_INFO("tcp_client", "connect", "Connected to server");
        })
        .on_data([](const std::string& data) {
            UNILINK_LOG_DEBUG("tcp_client", "data", "Data received: " + data);
        })
        .on_error([](const std::string& error) {
            UNILINK_LOG_ERROR("tcp_client", "error", "Client error: " + error);
        })
        .build();
    
    if (client) {
        UNILINK_LOG_INFO("example", "client", "TCP client created");
    }
    
    // 5. Serial communication logging
    std::cout << "\n5. Serial communication logging" << std::endl;
    
    auto serial_device = serial("/dev/ttyUSB0", 115200)
        .auto_start(false)  // Don't start since no real device
        .on_connect([]() {
            UNILINK_LOG_INFO("serial", "connect", "Serial device connected");
        })
        .on_data([](const std::string& data) {
            UNILINK_LOG_DEBUG("serial", "data", "Serial data received: " + data);
        })
        .on_error([](const std::string& error) {
            UNILINK_LOG_ERROR("serial", "error", "Serial error: " + error);
        })
        .build();
    
    if (serial_device) {
        UNILINK_LOG_INFO("example", "serial", "Serial device created");
    }
    
    // 6. Performance logging
    std::cout << "\n6. Performance logging" << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulation
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    UNILINK_LOG_DEBUG("example", "data_processing", "Duration: " + std::to_string(duration) + " μs");
    
    // 7. Log level change test
    std::cout << "\n7. Log level change test" << std::endl;
    
    UNILINK_LOG_DEBUG("example", "test", "This message is DEBUG level");
    
    // Change to INFO level
    common::Logger::instance().set_level(common::LogLevel::INFO);
    UNILINK_LOG_DEBUG("example", "test", "This message is not visible (DEBUG level)");
    UNILINK_LOG_INFO("example", "test", "This message is visible (INFO level)");
    
    // 8. Disable log output
    std::cout << "\n8. Disable log output" << std::endl;
    
    common::Logger::instance().set_enabled(false);
    UNILINK_LOG_INFO("example", "test", "This message is not visible (logging disabled)");
    
    common::Logger::instance().set_enabled(true);
    UNILINK_LOG_INFO("example", "test", "Logging re-enabled");
    
    // 9. Cleanup
    std::cout << "\n9. Cleanup" << std::endl;
    
    if (client) client->stop();
    if (server) server->stop();
    
    UNILINK_LOG_INFO("example", "shutdown", "Application shutdown");
    
    // Flush log file
    common::Logger::instance().flush();
    
    std::cout << "\n=== Logging example completed ===" << std::endl;
    std::cout << "Log file: unilink_example.log" << std::endl;
    
    return 0;
}
