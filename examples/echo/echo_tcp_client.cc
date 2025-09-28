#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

/**
 * @brief Example TCP client application using member function binding
 */
class TcpEchoClientApp {
public:
    TcpEchoClientApp(const std::string& host, uint16_t port) 
        : host_(host), port_(port), connected_(false), stop_sending_(false) {
    }

    void run() {
        // Using builder pattern with member function pointers
        auto client = unilink::builder::UnifiedBuilder::tcp_client(host_, port_)
            .auto_start(false)
            .on_connect(this, &TcpEchoClientApp::handle_connect)      // Member function binding
            .on_disconnect(this, &TcpEchoClientApp::handle_disconnect) // Member function binding
            .on_data(this, &TcpEchoClientApp::handle_data)            // Member function binding
            .on_error(this, &TcpEchoClientApp::handle_error)          // Member function binding
            .build();

        // Start sender thread
        std::thread sender_thread([this, &client] {
            this->sender_loop(client.get());
        });

        // Start the TCP client
        client->start();

        // Wait for program termination
        std::promise<void>().get_future().wait();

        // Cleanup
        stop_sending_ = true;
        client->stop();
        sender_thread.join();
    }

private:
    // Member function callbacks
    void handle_connect() {
        unilink::log_message("[tcp_client]", "STATE", "Connected to " + host_ + ":" + std::to_string(port_));
        connected_ = true;
    }

    void handle_disconnect() {
        unilink::log_message("[tcp_client]", "STATE", "Disconnected from server");
        connected_ = false;
    }

    void handle_data(const std::string& data) {
        unilink::log_message("[tcp_client]", "RX", data);
    }

    void handle_error(const std::string& error) {
        unilink::log_message("[tcp_client]", "ERROR", error);
    }

    void sender_loop(unilink::wrapper::TcpClient* client) {
        uint64_t seq = 0;
        const auto interval = std::chrono::milliseconds(1000);
        
        while (!stop_sending_) {
            if (connected_) {
                std::string msg = "TCP_CLIENT " + std::to_string(seq++);
                unilink::log_message("[tcp_client]", "TX", msg);
                client->send_line(msg);
            }
            std::this_thread::sleep_for(interval);
        }
    }

    std::string host_;
    uint16_t port_;
    std::atomic<bool> connected_;
    std::atomic<bool> stop_sending_;
};

int main(int argc, char** argv) {
    std::string host = (argc > 1) ? argv[1] : "localhost";
    uint16_t port = (argc > 2) ? static_cast<uint16_t>(std::stoi(argv[2])) : 8080;
    
    TcpEchoClientApp app(host, port);
    app.run();
    
    return 0;
}
