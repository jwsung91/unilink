#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : std::string("127.0.0.1");
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

  std::atomic<bool> stop_sending = false;
  std::thread sender_thread([&ul, &connected, &stop_sending] {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(1000);
    while (!stop_sending) {
      if (connected) {
        auto msg = "HELLO " + std::to_string(seq++);
        unilink::log_message("[client]", "TX", msg);
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
