#pragma once
#include <memory>
#include <string>

#include "ichannel.hpp"

struct SerialConfig {
  unsigned baud_rate = 115200;
  unsigned char_size = 8;  // 5,6,7,8
  enum class Parity { None, Even, Odd } parity = Parity::None;
  unsigned stop_bits = 1;  // 1 or 2
  enum class Flow { None, Software, Hardware } flow = Flow::None;

  size_t read_chunk = 4096;     // 1회 read 버퍼 크기
  bool reopen_on_error = true;  // 장치 분리/에러 시 재오픈 시도
};

std::shared_ptr<IChannel> make_tcp_client(class boost::asio::io_context& ioc,
                                          const std::string& host,
                                          uint16_t port);
std::shared_ptr<IChannel> make_tcp_server_single(
    class boost::asio::io_context& ioc, uint16_t port);

std::shared_ptr<IChannel> make_serial_channel(
    class boost::asio::io_context& ioc,
    const std::string& device,  // 예: "/dev/ttyUSB0"
    const SerialConfig& cfg);
