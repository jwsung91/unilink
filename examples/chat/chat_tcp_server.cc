// examples/raw_server.cc (교체)
#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <chrono>
#include <thread>

#include "unilink/unilink.hpp"

std::atomic<bool> running{true};

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    unilink::log_message("[server]", "INFO", "Received shutdown signal");
    running = false;
  }
}

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  
  // 시그널 핸들러 설정
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  
  std::atomic<bool> connected{false};

  // Using convenience function for configuration
  auto ul = unilink::tcp_server(port)
      .auto_start(false)
      .on_connect([&]() {
          unilink::log_message("[server]", "STATE", "Client connected");
          connected = true;
      })
      .on_disconnect([&]() {
          unilink::log_message("[server]", "STATE", "Client disconnected");
          connected = false;
      })
      .on_data([&](const std::string& data) {
          unilink::log_message("[server]", "RX", data);
      })
      .build();

  // ⬇️ 서버에서 키보드 입력 → 클라이언트로 전송
  std::thread input_thread([&ul, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        unilink::log_message("[server]", "INFO", "(not connected)");
        continue;
      }
      unilink::log_message("[server]", "TX", line);
      ul->send_line(line);
    }
  });

  ul->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  while (running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  unilink::log_message("[server]", "INFO", "Shutting down server...");
  input_thread.join();
  ul->stop();
  unilink::log_message("[server]", "INFO", "Server stopped");

  return 0;
}
