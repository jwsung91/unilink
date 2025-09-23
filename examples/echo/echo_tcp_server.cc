#include <future>
#include <iostream>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;

  unilink::TcpServerConfig cfg{port};
  auto ch = unilink::create(cfg);
  ch->on_state([&](unilink::LinkState s) {
    auto state_msg = "state=" + std::string(unilink::to_cstr(s));
    unilink::log_message("[server]", "STATE", state_msg);
  });

  ch->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    unilink::log_message("[server]", "RX", s);
    unilink::log_message("[server]", "TX", s);
    ch->async_write_copy(p, n);
  });

  ch->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  std::promise<void>().get_future().wait();

  ch->stop();
  return 0;
}
