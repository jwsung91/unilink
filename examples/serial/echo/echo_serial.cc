/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

// Example namespace usage - using namespace for simplicity in examples
using namespace unilink;

/**
 * @brief Example application class that demonstrates member function binding
 *
 * This class shows how to use the new member function pointer overloads
 * in the builder pattern to bind callback methods directly to class methods.
 */
class SerialEchoApp {
 public:
  SerialEchoApp(const std::string& device, uint32_t baud_rate)
      : device_(device), baud_rate_(baud_rate), connected_(false), stop_sending_(false) {}

  void run() {
    // Using convenience function with member function pointers
    auto ul = unilink::serial(device_, baud_rate_)
                  .auto_start(false)
                  .on_connect(this, &SerialEchoApp::handle_connect)
                  .on_disconnect(this, &SerialEchoApp::handle_disconnect)
                  .on_data(this, &SerialEchoApp::handle_data)
                  .on_error(this, &SerialEchoApp::handle_error)
                  .retry_interval(5000)
                  .build();

    // Start sender thread
    std::thread sender_thread([this, &ul] { this->sender_loop(ul.get()); });

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
    common::log_message("serial", "STATE", "Serial device connected");
    connected_ = true;
  }

  void handle_disconnect() {
    common::log_message("serial", "STATE", "Serial device disconnected");
    connected_ = false;
  }

  void handle_data(const std::string& data) { common::log_message("serial", "RX", data); }

  void handle_error(const std::string& error) { common::log_message("serial", "ERROR", error); }

  void sender_loop(unilink::wrapper::Serial* serial) {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(500);

    while (!stop_sending_) {
      if (connected_) {
        std::string msg = "SER " + std::to_string(seq++);
        common::log_message("serial", "TX", msg);
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
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <device> <baud_rate>" << std::endl;
    std::cout << "Example: " << argv[0] << " /dev/ttyUSB0 115200" << std::endl;
    std::cout << "Example: " << argv[0] << " COM3 9600" << std::endl;
    return 1;
  }

  std::string dev = argv[1];
  uint32_t baud_rate = static_cast<uint32_t>(std::stoi(argv[2]));

  SerialEchoApp app(dev, baud_rate);
  app.run();

  return 0;
}
