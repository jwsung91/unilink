#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

using namespace unilink;

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  SerialConfig cfg;
  cfg.device = dev;
  cfg.baud_rate = 115200;
  cfg.retry_interval_ms = 2000;

  auto ch = create(cfg);

  std::atomic<bool> connected{false};
  std::string rx_acc;

  ch->on_state([&](LinkState s) {
    std::string state_msg = "state=" + std::string(to_cstr(s));
    log_message("[serial]", "STATE", state_msg);
    connected = (s == LinkState::Connected);
  });

  ch->on_bytes([&](const uint8_t* p, size_t n) {
    feed_lines(rx_acc, p, n,
               [&](std::string line) { log_message("[serial]", "RX", line); });
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 포트로 전송
  std::thread input_thread([ch, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        log_message("[serial]", "INFO", "(not connected)");
        continue;
      }
      std::string msg = line + "\n";
      log_message("[serial]", "TX", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      ch->async_write_copy(buf.data(), buf.size());
    }
  });

  ch->start();

  input_thread.join();
  ch->stop();

  return 0;
}
