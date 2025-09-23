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

  unilink::TcpClientConfig cfg{};
  cfg.host = host;
  cfg.port = port;
  auto cli = unilink::create(cfg);

  std::atomic<bool> connected{false};

  cli->on_state([&](unilink::LinkState s) {
    auto state_msg = "state=" + std::string(unilink::to_cstr(s));
    unilink::log_message("[client]", "STATE", state_msg);
    connected = (s == unilink::LinkState::Connected);
  });

  cli->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    unilink::log_message("[client]", "RX", s);
  });

  std::atomic<bool> stop_sending = false;
  std::thread sender_thread([cli, &connected, &stop_sending] {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(1000);
    while (!stop_sending) {
      if (connected) {
        auto msg = "HELLO " + std::to_string(seq++) + "\n";
        std::vector<uint8_t> buf(msg.begin(), msg.end());
        unilink::log_message("[client]", "TX", msg);
        cli->async_write_copy(buf.data(), buf.size());
      }
      std::this_thread::sleep_for(interval);
    }
  });

  cli->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  std::promise<void>().get_future().wait();

  stop_sending = true;
  cli->stop();
  sender_thread.join();
  return 0;
}
