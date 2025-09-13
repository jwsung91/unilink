#include "config.hpp"
#ifdef INTERFACE_SOCKET_ENABLE_YAML
#include <yaml-cpp/yaml.h>
#endif
#include <stdexcept>

#ifdef INTERFACE_SOCKET_ENABLE_YAML
static int get_or(const YAML::Node& n, const char* key, int defv) {
    if (n[key]) return n[key].as<int>();
    return defv;
}
static std::string get_or(const YAML::Node& n, const char* key,
                          const std::string& defv) {
    if (n[key]) return n[key].as<std::string>();
    return defv;
}
#endif

SocketConfig load_config_from_yaml(const std::string& path) {
#ifndef INTERFACE_SOCKET_ENABLE_YAML
    (void)path;
    throw std::runtime_error(
        "YAML support disabled. Build with -DENABLE_YAML_CONFIG=ON");
#else
    YAML::Node root = YAML::LoadFile(path);
    SocketConfig c;
    if (root["mode"]) c.mode = root["mode"].as<std::string>();
    if (root["host"]) c.host = root["host"].as<std::string>();
    if (root["port"]) c.port = root["port"].as<uint16_t>();

    c.request_timeout_ms =
        get_or(root, "request_timeout_ms", c.request_timeout_ms);
    c.idle_timeout_ms = get_or(root, "idle_timeout_ms", c.idle_timeout_ms);
    c.reconnect_backoff_initial_ms = get_or(
        root, "reconnect_backoff_initial_ms", c.reconnect_backoff_initial_ms);
    c.reconnect_backoff_max_ms =
        get_or(root, "reconnect_backoff_max_ms", c.reconnect_backoff_max_ms);
    c.accept_backoff_initial_ms =
        get_or(root, "accept_backoff_initial_ms", c.accept_backoff_initial_ms);
    c.write_queue_limit =
        get_or(root, "write_queue_limit", c.write_queue_limit);
    c.max_packet_bytes = get_or(root, "max_packet_bytes", c.max_packet_bytes);
    return c;
#endif
}