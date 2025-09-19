#pragma once
#include <cstddef>

struct SerialConfig {
  unsigned baud_rate = 115200;
  unsigned char_size = 8;  // 5,6,7,8
  enum class Parity { None, Even, Odd } parity = Parity::None;
  unsigned stop_bits = 1;  // 1 or 2
  enum class Flow { None, Software, Hardware } flow = Flow::None;

  size_t read_chunk = 4096;     // 1회 read 버퍼 크기
  bool reopen_on_error = true;  // 장치 분리/에러 시 재오픈 시도

  unsigned retry_interval_ms = 2000;  // 2s
};