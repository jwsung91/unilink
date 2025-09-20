#include <atomic>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "factory/channel_factory.hpp"
#include "interface/ichannel.hpp"

using namespace unilink::interface;
using namespace unilink::factory;
using namespace unilink::common;
using namespace unilink::config;

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
  unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 9000;

  TcpClientConfig cfg;
  cfg.host = host;
  cfg.port = port;

  auto cli = ChannelFactory::create(cfg);

  std::atomic<bool> connected{false};
  std::string rx_acc;

  cli->on_state([&](LinkState s) {
    std::string state_msg = "state=" + std::string(to_cstr(s));
    log_message("[client]", "STATE", state_msg);
    connected = (s == LinkState::Connected);
  });

  cli->on_bytes([&](const uint8_t* p, size_t n) {
    feed_lines(rx_acc, p, n,
               [&](std::string line) { log_message("[client]", "RX", line); });
  });

  // 입력 쓰레드: stdin 한 줄 읽어서 서버로 전송
  std::thread([cli, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        log_message("[client]", "INFO", "(not connected)");
        continue;
      }
      std::string msg = line + "\n";
      log_message("[client]", "TX", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      cli->async_write_copy(buf.data(), buf.size());
    }
  }).detach();

  cli->start();
  return 0;
}
