#include <boost/asio.hpp>
#include <iostream>

#include "common/common.hpp"
#include "factory/channel_factory.hpp"
#include "interface/ichannel.hpp"

// ts_now() is assumed to be defined elsewhere, returning a timestamp string.
void log_message(const std::string& tag, const std::string& direction,
                 const std::string& message) {
  std::cout << ts_now() << " " << tag << " [" << direction << "] " << message
            << std::endl;
}

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;
  boost::asio::io_context ioc;

  TcpServerConfig cfg{port};
  auto srv = ChannelFactory::create(ioc, cfg);
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
  ioc.run();
  return 0;
}
