#pragma once
#include <chrono>
#include <cstdint>
#include <string>

struct SocketConfig {
    std::string mode = "server";  // "server" or "client"
    std::string host = "127.0.0.1";
    uint16_t port = 9000;

    int request_timeout_ms = 1500;
    int idle_timeout_ms = 0;
    int reconnect_backoff_initial_ms = 1000;
    int reconnect_backoff_max_ms = 30000;
    int accept_backoff_initial_ms = 1000;

    int write_queue_limit = 1024;
    int max_packet_bytes = 65535;
};

SocketConfig load_config_from_yaml(const std::string& path);