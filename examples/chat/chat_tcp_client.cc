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

  unilink::TcpClientConfig cfg;
  cfg.host = host;
  cfg.port = port;

  auto cli = unilink::create(cfg);

  std::atomic<bool> connected{false};
  std::string rx_acc;

  cli->on_state([&](unilink::LinkState s) {
    auto state_msg = "state=" + std::string(unilink::to_cstr(s));
    unilink::log_message("[client]", "STATE", state_msg);
    connected = (s == unilink::LinkState::Connected);
  });

  cli->on_bytes([&](const uint8_t* p, size_t n) {
    unilink::feed_lines(rx_acc, p, n, [&](std::string line) {
      unilink::log_message("[client]", "RX", line);
    });
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 서버로 전송
  std::thread input_thread([cli, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        unilink::log_message("[client]", "INFO", "(not connected)");
        continue;
      }
      auto msg = line + "\n";
      unilink::log_message("[client]", "TX", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      cli->async_write_copy(buf.data(), buf.size());
    }
  });

  cli->start();

  input_thread.join();
  cli->stop();

  return 0;
}
