#include <iostream>
#include <boost/asio.hpp>
#include "factory.hpp"

int main(int argc, char** argv) {
  unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  boost::asio::io_context ioc;
  auto ch = make_server_single(ioc, port);
  ch->on_state([](LinkState s){ std::cout << "[server] state=" << (int)s << "\n"; });
  ch->on_receive([&](const Msg& m){
    ch->async_send(m); // echo back
  });
  ch->start();
  ioc.run();
  return 0;
}
