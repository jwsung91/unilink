#include <boost/asio.hpp>
#include <iostream>

#include "common/common.hpp"
#include "factory/channel_factory.hpp"
#include "interface/ichannel.hpp"

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  boost::asio::io_context ioc;

  TcpServerConfig cfg{port};
  auto srv = ChannelFactory::create(ioc, cfg);
  srv->on_state([](LinkState s) {
    std::cout << ts_now() << " [server] state=" << to_cstr(s) << std::endl;
  });

  srv->on_bytes([&](const uint8_t* p, size_t n) {
    std::string s(reinterpret_cast<const char*>(p), n);
    std::cout << ts_now() << " [server] recv chunk: " << s;
  });

  srv->start();
  ioc.run();
  return 0;
}
