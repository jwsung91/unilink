// examples/raw_server.cc (교체)
#include <atomic>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"
#include "factory.hpp"
#include "ichannel.hpp"

// (선택) 간단 로깅 헬퍼
#include <sstream>
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

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  boost::asio::io_context ioc;

  auto srv = ChannelFactory::make_tcp_server_single(ioc, port);

  std::atomic<bool> connected{false};

  srv->on_state([&](LinkState s) {
    std::cout << ts_now() << " [server] state=" << to_cstr(s) << std::endl;
    connected = (s == LinkState::Connected);
  });

  srv->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    LOG_RX("[server]", s);
  });

  // ⬇️ 서버에서 키보드 입력 → 클라이언트로 전송
  std::thread([srv, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        std::cout << ts_now() << " [server] (not connected)\n";
        continue;
      }
      std::string msg = line + "\n";
      LOG_TX("[server]", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      srv->async_write_copy(buf.data(), buf.size());
    }
  }).detach();

  srv->start();
  ioc.run();
  return 0;
}
