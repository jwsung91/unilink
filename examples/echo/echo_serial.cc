#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

/**
 * @brief Example application class that demonstrates member function binding
 * 
 * This class shows how to use the new member function pointer overloads
 * in the builder pattern to bind callback methods directly to class methods.
 */
class SerialEchoApp {
public:
    SerialEchoApp(const std::string& device, uint32_t baud_rate) 
        : device_(device), baud_rate_(baud_rate), connected_(false), stop_sending_(false) {
    }

    void run() {
        // Using convenience function with member function pointers
        auto ul = unilink::serial(device_, baud_rate_)
            .auto_start(false)
            .on_connect(this, &SerialEchoApp::handle_connect)      // Member function binding
            .on_disconnect(this, &SerialEchoApp::handle_disconnect) // Member function binding
            .on_data(this, &SerialEchoApp::handle_data)            // Member function binding
            .on_error(this, &SerialEchoApp::handle_error)          // Member function binding
            .build();

        // Start sender thread
        std::thread sender_thread([this, &ul] {
            this->sender_loop(ul.get());
        });

        // Start the serial connection
        ul->start();

        // Wait for program termination
        std::promise<void>().get_future().wait();

        // Cleanup
        stop_sending_ = true;
        ul->stop();
        sender_thread.join();
    }

private:
    // Member function callbacks - these can be bound directly to the builder
    void handle_connect() {
        unilink::log_message("[serial]", "STATE", "Serial device connected");
        connected_ = true;
    }

    void handle_disconnect() {
        unilink::log_message("[serial]", "STATE", "Serial device disconnected");
        connected_ = false;
    }

    void handle_data(const std::string& data) {
        unilink::log_message("[serial]", "RX", data);
    }

    void handle_error(const std::string& error) {
        unilink::log_message("[serial]", "ERROR", error);
    }

    void sender_loop(unilink::wrapper::Serial* serial) {
        uint64_t seq = 0;
        const auto interval = std::chrono::milliseconds(500);
        
        while (!stop_sending_) {
            if (connected_) {
                std::string msg = "SER " + std::to_string(seq++);
                unilink::log_message("[serial]", "TX", msg);
                serial->send_line(msg);
            }
            std::this_thread::sleep_for(interval);
        }
    }

    std::string device_;
    uint32_t baud_rate_;
    std::atomic<bool> connected_;
    std::atomic<bool> stop_sending_;
};

int main(int argc, char** argv) {
    std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";
    
    SerialEchoApp app(dev, 115200);
    app.run();
    
    return 0;
}
