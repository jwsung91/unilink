#pragma once
#include <vector>
#include <cstdint>
#include <functional>
#include <chrono>

struct Msg {
    std::vector<uint8_t> bytes; // payload
    uint32_t seq = 0;           // correlation id
};

enum class LinkState { Idle, Connecting, Listening, Connected, Closed, Error };

using Clock = std::chrono::steady_clock;
