#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

/**
 * @brief Chat application class for serial communication
 *
 * This class demonstrates member function binding for serial chat functionality.
 */
class SerialChatApp {
 public:
  SerialChatApp(const std::string& device, uint32_t baud_rate)
      : device_(device), baud_rate_(baud_rate), connected_(false) {}

  void run() {
    // Using convenience function with member function pointers
    auto ul = unilink::serial(device_, baud_rate_)
                  .auto_start(false)
                  .on_connect(this, &SerialChatApp::handle_connect)
                  .on_disconnect(this, &SerialChatApp::handle_disconnect)
                  .on_data(this, &SerialChatApp::handle_data)
                  .on_error(this, &SerialChatApp::handle_error)
                  .build();

    // Start input thread
    std::thread input_thread([this, &ul] { this->input_loop(ul.get()); });

    // Start the serial connection
    ul->start();

    // Wait for input thread to finish
    input_thread.join();
    ul->stop();
  }

 private:
  // Member function callbacks - these can be bound directly to the builder
  void handle_connect() {
    unilink::log_message("serial", "STATE", "Serial device connected");
    connected_ = true;
  }

  void handle_disconnect() {
    unilink::log_message("serial", "STATE", "Serial device disconnected");
    connected_ = false;
  }

  void handle_data(const std::string& data) { unilink::log_message("serial", "RX", data); }

  void handle_error(const std::string& error) { unilink::log_message("serial", "ERROR", error); }

  void input_loop(unilink::wrapper::Serial* serial) {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected_.load()) {
        unilink::log_message("serial", "INFO", "(not connected)");
        continue;
      }
      unilink::log_message("serial", "TX", line);
      serial->send_line(line);
    }
  }

  std::string device_;
  uint32_t baud_rate_;
  std::atomic<bool> connected_;
};

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  SerialChatApp app(dev, 115200);
  app.run();

  return 0;
}
