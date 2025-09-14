#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>

#include "factory.hpp"

int main(int argc, char** argv) {
  std::string dev = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  boost::asio::io_context ioc;
  SerialConfig cfg;
  cfg.baud_rate = 115200;

  auto ser = make_serial_channel(ioc, dev, cfg);

  ser->on_state([](LinkState s) {
    std::cout << "[serial] state=" << to_cstr(s) << "\n";
  });
  ser->on_bytes([&](const uint8_t* p, size_t n) {
    std::cout << "[serial] recv " << n << " bytes\n";
    // 에코
    ser->async_write_copy(p, n);
  });

  ser->start();

  // 연결 후 테스트 송신
  std::thread([ser] {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const char* msg = "hello-serial";
    ser->async_write_copy(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
  }).detach();

  ioc.run();
}