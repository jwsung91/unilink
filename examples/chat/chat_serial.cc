#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  unilink::SerialConfig cfg;
  cfg.device = dev;
  cfg.baud_rate = 115200;
  cfg.retry_interval_ms = 2000;

  auto ul = unilink::create(cfg);

  std::atomic<bool> connected{false};
  std::string rx_acc;

  ul->on_state([&](unilink::LinkState s) {
    auto state_msg = "state=" + std::string(unilink::to_cstr(s));
    unilink::log_message("[serial]", "STATE", state_msg);
    connected = (s == unilink::LinkState::Connected);
  });

  ul->on_bytes([&](const uint8_t* p, size_t n) {
    unilink::feed_lines(rx_acc, p, n, [&](std::string line) {
      unilink::log_message("[serial]", "RX", line);
    });
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 포트로 전송
  std::thread input_thread([ul, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        unilink::log_message("[serial]", "INFO", "(not connected)");
        continue;
      }
      auto msg = line + "\n";
      unilink::log_message("[serial]", "TX", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      ul->async_write_copy(buf.data(), buf.size());
    }
  });

  ul->start();

  input_thread.join();
  ul->stop();

  return 0;
}
