
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"
#include "factory.hpp"
#include "ichannel.hpp"

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : std::string("127.0.0.1");
  unsigned short port =
      (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 9000;

  boost::asio::io_context ioc;
  auto cli = make_tcp_client(ioc, host, port);

  cli->on_state([&](LinkState s) {
    std::cout << "[client] state=" << to_cstr(s) << std::endl;
    if (s == LinkState::Connected) {
      // Send three messages from a non-IO thread
      std::thread([cli] {
        for (int i = 0; i < 3; ++i) {
          std::string msg = "Hello " + std::to_string(i);
          std::vector<uint8_t> buf(msg.begin(), msg.end());
          cli->async_write_copy(buf.data(), buf.size());
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }).detach();
    }
  });

  cli->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    std::cout << "[client] recv: " << s << std::endl;
  });

  cli->start();
  ioc.run();
  return 0;
}
