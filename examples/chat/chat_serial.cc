#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "common/common.hpp"
#include "factory/channel_factory.hpp"

// ts_now() is assumed to be defined elsewhere, returning a timestamp string.
void log_message(const std::string& tag, const std::string& direction,
                 const std::string& message) {
  std::cout << ts_now() << " " << tag << " [" << direction << "] " << message
            << std::endl;
}

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
  cfg.device = dev;
  cfg.baud_rate = 115200;
  cfg.retry_interval_ms = 2000;

  auto ch = ChannelFactory::create(ioc, cfg);

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
  std::thread([ch, &connected] {
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
  }).detach();

  ch->start();
  ioc.run();
  return 0;
}
