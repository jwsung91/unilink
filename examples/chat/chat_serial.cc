#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "channel_factory.hpp"
#include "common.hpp"

#define LOG_TX(TAG, X)                                             \
  do {                                                             \
    std::ostringstream __oss;                                      \
    __oss << X;                                                    \
    std::cout << ts_now() << " " << TAG << " [TX] " << __oss.str() \
              << std::endl;                                        \
  } while (0)
#define LOG_RX(TAG, X)                                             \
  do {                                                             \
    std::ostringstream __oss;                                      \
    __oss << X;                                                    \
    std::cout << ts_now() << " " << TAG << " [RX] " << __oss.str() \
              << std::endl;                                        \
  } while (0)
static void feed_lines(std::string& acc, const uint8_t* p, size_t n,
                       const std::function<void(std::string)>& on_line) {
  acc.append(reinterpret_cast<const char*>(p), n);
  size_t pos;
  while ((pos = acc.find('\n')) != std::string::npos) {
    std::string line = acc.substr(0, pos);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    on_line(line);
    acc.erase(0, pos + 1);
  }
}

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  boost::asio::io_context ioc;
  SerialConfig cfg;
  cfg.baud_rate = 115200;
  cfg.retry_interval_ms = 2000;

  SerialOptions opts{dev, cfg};
  auto ch = ChannelFactory::create(ioc, opts);

  std::atomic<bool> connected{false};
  std::string rx_acc;

  ch->on_state([&](LinkState s) {
    std::cout << ts_now() << " [serial] state=" << to_cstr(s) << std::endl;
    connected = (s == LinkState::Connected);
  });

  ch->on_bytes([&](const uint8_t* p, size_t n) {
    feed_lines(rx_acc, p, n,
               [&](std::string line) { LOG_RX("[serial]", line); });
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 포트로 전송
  std::thread([ch, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        std::cout << ts_now() << " [serial] (not connected)\n";
        continue;
      }
      std::string msg = line + "\n";
      LOG_TX("[serial]", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      ch->async_write_copy(buf.data(), buf.size());
    }
  }).detach();

  ch->start();
  ioc.run();
  return 0;
}
