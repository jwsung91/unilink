#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "factory.hpp"

int main(int argc, char** argv) {
  std::string host = (argc > 1) ? argv[1] : std::string("127.0.0.1");
  unsigned short port = (argc > 2) ? static_cast<unsigned short>(std::stoi(argv[2])) : 9000;

  boost::asio::io_context ioc;
  auto ch = make_client_single(ioc, host, port);
  ch->on_state([](LinkState s){ std::cout << "[client] state=" << (int)s << "\n"; });
  ch->start();

  boost::asio::steady_timer t(ioc);
  t.expires_after(std::chrono::milliseconds(300));
  t.async_wait([&](auto){
    for (int i=0;i<3;++i) {
      Msg m; std::string payload = std::string("Hello ") + std::to_string(i);
      m.bytes.assign(payload.begin(), payload.end());
      try {
        auto resp = ch->request(m, std::chrono::milliseconds(1500)).get();
        std::string s(resp.bytes.begin(), resp.bytes.end());
        std::cout << "[client] response seq=" << resp.seq << ": " << s << "\n";
      } catch (const std::exception& e) {
        std::cout << "[client] request error: " << e.what() << "\n";
      }
    }
  });

  ioc.run();
  return 0;
}
