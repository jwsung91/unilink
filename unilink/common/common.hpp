#pragma once

#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace unilink {
namespace common {

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
  return oss.str();  // e.g., 2025-09-15 13:07:42.123
}

inline void log_message(const std::string& tag, const std::string& direction,
                        const std::string& message) {
  // Remove trailing newline from message if it exists
  std::string clean_message = message;
  if (!clean_message.empty() && clean_message.back() == '\n') {
    clean_message.pop_back();
  }
  std::cout << ts_now() << " " << tag << " [" << direction << "] "
            << clean_message << std::endl;
}

static void feed_lines(std::string& acc, const uint8_t* p, size_t n,
                       const std::function<void(std::string)>& on_line) {
  acc.append(reinterpret_cast<const char*>(p), n);
  size_t pos;
  while ((pos = acc.find('\n')) != std::string::npos) {
    std::string line = acc.substr(0, pos);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    on_line(line);
    acc.erase(0, pos + 1);
  }
}
}  // namespace common
}  // namespace unilink