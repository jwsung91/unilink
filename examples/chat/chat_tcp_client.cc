#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"
#include "factory.hpp"
#include "ichannel.hpp"

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
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 9000;

  boost::asio::io_context ioc;
  auto cli = make_tcp_client(ioc, host, port);

  std::atomic<bool> connected{false};
  std::string rx_acc;

  cli->on_state([&](LinkState s) {
    std::cout << ts_now() << " [client] state=" << to_cstr(s) << std::endl;
    connected = (s == LinkState::Connected);
  });

  cli->on_bytes([&](const uint8_t* p, size_t n) {
    feed_lines(rx_acc, p, n,
               [&](std::string line) { LOG_RX("[client]", line); });
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 서버로 전송
  std::thread([cli, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        std::cout << ts_now() << " [client] (not connected)\n";
        continue;
      }
      std::string msg = line + "\n";
      LOG_TX("[client]", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      cli->async_write_copy(buf.data(), buf.size());
    }
  }).detach();

  cli->start();
  ioc.run();
  return 0;
}
