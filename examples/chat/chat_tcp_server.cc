// examples/raw_server.cc (교체)
#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  std::atomic<bool> connected{false};

  // Using builder pattern for configuration
  auto ul = unilink::builder::UnifiedBuilder::tcp_server(port)
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

  input_thread.join();
  ul->stop();

  return 0;
}
