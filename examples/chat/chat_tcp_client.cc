#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 9000;

  // Using builder pattern for configuration
  auto ul = unilink::builder::UnifiedBuilder::tcp_client(host, port)
      .auto_start(false)
      .on_connect([&]() {
          unilink::log_message("[client]", "INFO", "Connected to server");
      })
      .on_disconnect([&]() {
          unilink::log_message("[client]", "INFO", "Disconnected from server");
      })
      .build();

  std::atomic<bool> connected{false};

  ul->on_connect([&]() {
    unilink::log_message("[client]", "STATE", "Connected");
    connected = true;
  });

  ul->on_disconnect([&]() {
    unilink::log_message("[client]", "STATE", "Disconnected");
    connected = false;
  });

  ul->on_data([&](const std::string& data) {
    unilink::log_message("[client]", "RX", data);
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 서버로 전송
  std::thread input_thread([&ul, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        unilink::log_message("[client]", "INFO", "(not connected)");
        continue;
      }
      unilink::log_message("[client]", "TX", line);
      ul->send_line(line);
    }
  });

  ul->start();

  input_thread.join();
  ul->stop();

  return 0;
}
