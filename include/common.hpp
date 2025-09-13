#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

struct Msg {
  std::vector<uint8_t> bytes;  // payload
  uint32_t seq = 0;            // correlation id
};

enum class LinkState { Idle, Connecting, Listening, Connected, Closed, Error };

using Clock = std::chrono::steady_clock;

// pretty state string
inline const char* to_cstr(LinkState s) {
  switch (s) {
    case LinkState::Idle:
      return "Idle";
    case LinkState::Connecting:
      return "Connecting";
    case LinkState::Listening:
      return "Listening";
    case LinkState::Connected:
      return "Connected";
    case LinkState::Closed:
      return "Closed";
    case LinkState::Error:
      return "Error";
  }
  return "?";
}