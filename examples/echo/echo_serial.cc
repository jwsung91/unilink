#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  // Using builder pattern for configuration
  auto ul = unilink::builder::UnifiedBuilder::serial(dev, 115200)
      .auto_start(false)
      .on_connect([&]() {
          unilink::log_message("[serial]", "INFO", "Serial device connected");
      })
      .on_disconnect([&]() {
          unilink::log_message("[serial]", "INFO", "Serial device disconnected");
      })
      .build();

  std::atomic<bool> connected{false};

  ul->on_connect([&]() {
    unilink::log_message("[serial]", "STATE", "Connected");
    connected = true;
  });

  ul->on_disconnect([&]() {
    unilink::log_message("[serial]", "STATE", "Disconnected");
    connected = false;
  });

  ul->on_data([&](const std::string& data) {
    unilink::log_message("[serial]", "RX", data);
  });

  std::atomic<bool> stop_sending = false;
  std::thread sender_thread([&ul, &connected, &stop_sending] {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(500);
    while (!stop_sending) {
      if (connected) {
        std::string msg = "SER " + std::to_string(seq++);
        unilink::log_message("[serial]", "TX", msg);
        ul->send_line(msg);
      }
      std::this_thread::sleep_for(interval);
    }
  });

  ul->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  std::promise<void>().get_future().wait();

  stop_sending = true;
  ul->stop();
  sender_thread.join();
  return 0;
}
