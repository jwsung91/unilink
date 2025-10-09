#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

// Example namespace usage - using namespace for simplicity in examples
using namespace unilink;

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
    common::log_message("serial", "STATE", "Serial device connected");
    connected_ = true;
  }

  void handle_disconnect() {
    common::log_message("serial", "STATE", "Serial device disconnected");
    connected_ = false;
  }

  void handle_data(const std::string& data) { common::log_message("serial", "RX", data); }

  void handle_error(const std::string& error) { common::log_message("serial", "ERROR", error); }

  void input_loop(unilink::wrapper::Serial* serial) {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected_.load()) {
        common::log_message("serial", "INFO", "(not connected)");
        continue;
      }
      common::log_message("serial", "TX", line);
      serial->send_line(line);
    }
  }

  std::string device_;
  uint32_t baud_rate_;
  std::atomic<bool> connected_;
};

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <device> <baud_rate>" << std::endl;
    std::cout << "Example: " << argv[0] << " /dev/ttyUSB0 115200" << std::endl;
    std::cout << "Example: " << argv[0] << " COM3 9600" << std::endl;
    return 1;
  }

  std::string dev = argv[1];
  uint32_t baud_rate = static_cast<uint32_t>(std::stoi(argv[2]));

  SerialChatApp app(dev, baud_rate);
  app.run();

  return 0;
}
