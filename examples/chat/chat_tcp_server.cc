// examples/raw_server.cc (교체)
#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#include "common/common.hpp"
#include "factory/channel_factory.hpp"
#include "interface/ichannel.hpp"

using namespace unilink::interface;
using namespace unilink::factory;
using namespace unilink::common;
using namespace unilink::config;

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  TcpServerConfig cfg{port};
  auto srv = ChannelFactory::create(cfg);

  std::atomic<bool> connected{false};

  srv->on_state([&](LinkState s) {
    std::string state_msg = "state=" + std::string(to_cstr(s));
    log_message("[server]", "STATE", state_msg);
    connected = (s == LinkState::Connected);
  });

  srv->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    log_message("[server]", "RX", s);
  });

  // ⬇️ 서버에서 키보드 입력 → 클라이언트로 전송
  std::thread([&srv, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        log_message("[server]", "INFO", "(not connected)");
        continue;
      }
      std::string msg = line + "\n";
      log_message("[server]", "TX", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      srv->async_write_copy(buf.data(), buf.size());
    }
  }).detach();

  srv->start();
  return 0;
}
