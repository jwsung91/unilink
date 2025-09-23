#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
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
  auto ser = create(cfg);

  std::atomic<bool> connected{false};

  ser->on_state([&](LinkState s) {
    std::string state_msg = "state=" + std::string(to_cstr(s));
    log_message("[serial]", "STATE", state_msg);
    connected = (s == LinkState::Connected);
  });

  ser->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    log_message("[serial]", "RX", s);
  });

  std::atomic<bool> stop_sending = false;
  std::thread sender_thread([ser, &connected, &stop_sending] {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(500);
    while (!stop_sending) {
      if (connected) {
        std::string msg = "SER " + std::to_string(seq++) + "\n";
        std::vector<uint8_t> buf(msg.begin(), msg.end());

        log_message("[serial]", "TX", msg);
        ser->async_write_copy(buf.data(), buf.size());
      }
      std::this_thread::sleep_for(interval);
    }
  });

  ser->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  std::promise<void>().get_future().wait();

  stop_sending = true;
  ser->stop();
  sender_thread.join();
  return 0;
}
