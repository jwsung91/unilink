#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
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

  ul->on_state([&](unilink::LinkState s) {
    auto state_msg = "state=" + std::string(unilink::to_cstr(s));
    unilink::log_message("[serial]", "STATE", state_msg);
    connected = (s == unilink::LinkState::Connected);
  });

  ul->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    unilink::log_message("[serial]", "RX", s);
  });

  std::atomic<bool> stop_sending = false;
  std::thread sender_thread([ul, &connected, &stop_sending] {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(500);
    while (!stop_sending) {
      if (connected) {
        std::string msg = "SER " + std::to_string(seq++) + "\n";
        std::vector<uint8_t> buf(msg.begin(), msg.end());

        unilink::log_message("[serial]", "TX", msg);
        ul->async_write_copy(buf.data(), buf.size());
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
