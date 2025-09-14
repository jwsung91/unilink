#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>

enum class LinkState { Idle, Connecting, Listening, Connected, Closed, Error };

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

inline std::string ts_now() {
  using namespace std::chrono;
  const auto now = system_clock::now();
  const auto tt = system_clock::to_time_t(now);

  // 로컬타임(우분투): thread-safe
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif

  const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream oss;
  oss << std::put_time(&tm, "%F %T") << '.' << std::setw(3) << std::setfill('0')
      << ms.count();
  return oss.str();  // 예: 2025-09-15 13:07:42.123
}