#include <future>
#include <iostream>

#include "unilink/unilink.hpp"

using namespace unilink;

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;

  TcpServerConfig cfg{port};
  auto srv = create(cfg);
  srv->on_state([&](LinkState s) {
    std::string state_msg = "state=" + std::string(to_cstr(s));
    log_message("[server]", "STATE", state_msg);
  });

  srv->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    log_message("[server]", "RX", s);
    log_message("[server]", "TX", s);
    srv->async_write_copy(p, n);
  });

  srv->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  std::promise<void>().get_future().wait();

  srv->stop();
  return 0;
}
