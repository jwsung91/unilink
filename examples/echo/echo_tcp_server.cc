#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

/**
 * @brief Example TCP server application using member function binding
 */
class TcpEchoServerApp {
public:
    TcpEchoServerApp(uint16_t port) 
        : port_(port), running_(false) {
    }

    void run() {
        // Using convenience function with member function pointers
        auto server = unilink::tcp_server(port_)
            .auto_start(false)
            .on_connect(this, &TcpEchoServerApp::handle_connect)      // Member function binding
            .on_disconnect(this, &TcpEchoServerApp::handle_disconnect) // Member function binding
            .on_data(this, &TcpEchoServerApp::handle_data)            // Member function binding
            .on_error(this, &TcpEchoServerApp::handle_error)          // Member function binding
            .build();

        // Start the TCP server
        server->start();
        running_ = true;
        unilink::log_message("[tcp_server]", "STATE", "Server started on port " + std::to_string(port_));

        // Wait for program termination
        std::promise<void>().get_future().wait();

        // Cleanup
        running_ = false;
        server->stop();
    }

private:
    // Member function callbacks
    void handle_connect() {
        unilink::log_message("[tcp_server]", "STATE", "Client connected");
    }

    void handle_disconnect() {
        unilink::log_message("[tcp_server]", "STATE", "Client disconnected");
    }

    void handle_data(const std::string& data) {
        unilink::log_message("[tcp_server]", "RX", data);
        
        // Echo back the received data
        // Note: In a real application, you'd need access to the server instance
        // to send data back. This is a simplified example.
        unilink::log_message("[tcp_server]", "ECHO", "Would echo: " + data);
    }

    void handle_error(const std::string& error) {
        unilink::log_message("[tcp_server]", "ERROR", error);
    }

    uint16_t port_;
    std::atomic<bool> running_;
};

int main(int argc, char** argv) {
    uint16_t port = (argc > 1) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;
    
    TcpEchoServerApp app(port);
    app.run();
    
    return 0;
}
