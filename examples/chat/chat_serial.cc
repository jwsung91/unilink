#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  std::atomic<bool> connected{false};

  // Using builder pattern for configuration
  auto ul = unilink::builder::UnifiedBuilder::serial(dev, 115200)
      .auto_start(false)
      .on_connect([&]() {
          unilink::log_message("[serial]", "STATE", "Serial device connected");
          connected = true;
      })
      .on_disconnect([&]() {
          unilink::log_message("[serial]", "STATE", "Serial device disconnected");
          connected = false;
      })
      .on_data([&](const std::string& data) {
          unilink::log_message("[serial]", "RX", data);
      })
      .on_error([&](const std::string& error) {
          unilink::log_message("[serial]", "ERROR", error);
      })
      .build();

  // 입력 쓰레드: stdin 한 줄 읽어서 포트로 전송
  std::thread input_thread([&ul, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        unilink::log_message("[serial]", "INFO", "(not connected)");
        continue;
      }
      unilink::log_message("[serial]", "TX", line);
      ul->send_line(line);
    }
  });

  ul->start();

  input_thread.join();
  ul->stop();

  return 0;
}
