#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "channel_factory.hpp"
#include "common.hpp"

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  boost::asio::io_context ioc;
  SerialConfig cfg;
  cfg.device = dev;
  cfg.baud_rate = 115200;
  cfg.retry_interval_ms = 2000;
  auto ser = ChannelFactory::create(ioc, cfg);

  std::atomic<bool> connected{false};

  ser->on_state([&](LinkState s) {
    std::cout << "[serial] state=" << to_cstr(s) << "\n";
    connected = (s == LinkState::Connected);
  });

  ser->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    std::cout << "[serial] recv chunk: " << s;
  });

  std::thread([ser, &connected] {
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(500);
    while (true) {
      if (connected.load()) {
        std::string msg = "SER " + std::to_string(seq++) + "\n";
        std::vector<uint8_t> buf(msg.begin(), msg.end());

        // ✅ 보낸 로그 추가 (큐에 넣는 시점)
        std::cout << "[serial] send (" << buf.size() << "B): " << msg;

        ser->async_write_copy(buf.data(), buf.size());
      }
      std::this_thread::sleep_for(interval);
    }
  }).detach();

  ser->start();
  ioc.run();
  return 0;
}
