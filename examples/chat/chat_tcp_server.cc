// examples/raw_server.cc (교체)
#include <atomic>
#include <iostream>
#include <string>
#include <vector>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  unilink::TcpServerConfig cfg{port};
  auto ch = unilink::create(cfg);

  std::atomic<bool> connected{false};

  ch->on_state([&](unilink::LinkState s) {
    auto state_msg = "state=" + std::string(unilink::to_cstr(s));
    unilink::log_message("[server]", "STATE", state_msg);
    connected = (s == unilink::LinkState::Connected);
  });

  ch->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    unilink::log_message("[server]", "RX", s);
  });

  // ⬇️ 서버에서 키보드 입력 → 클라이언트로 전송
  std::thread input_thread([&ch, &connected] {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected.load()) {
        unilink::log_message("[server]", "INFO", "(not connected)");
        continue;
      }
      auto msg = line + "\n";
      unilink::log_message("[server]", "TX", line);
      std::vector<uint8_t> buf(msg.begin(), msg.end());
      ch->async_write_copy(buf.data(), buf.size());
    }
  });

  ch->start();

  input_thread.join();
  ch->stop();

  return 0;
}
