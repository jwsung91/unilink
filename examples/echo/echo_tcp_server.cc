#include <future>
#include <iostream>
#include <csignal>
#include <atomic>

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

  // Using builder pattern for configuration
  std::unique_ptr<unilink::wrapper::TcpServer> ul;
  
  ul = unilink::builder::UnifiedBuilder::tcp_server(port)
      .auto_start(false)
      .on_connect([&]() {
          unilink::log_message("[server]", "STATE", "Client connected");
      })
      .on_disconnect([&]() {
          unilink::log_message("[server]", "STATE", "Client disconnected");
      })
      .on_data([&](const std::string& data) {
          unilink::log_message("[server]", "RX", data);
          unilink::log_message("[server]", "TX", data);
          ul->send(data);
      })
      .build();

  ul->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  while (running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  unilink::log_message("[server]", "INFO", "Shutting down server...");
  ul->stop();
  unilink::log_message("[server]", "INFO", "Server stopped");
  return 0;
}
