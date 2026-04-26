/*
 * C++ native stability benchmark — Unilink wrapper API (no Python/GIL overhead)
 * Mirrors tools/benchmark/stability/stability_bench.py in structure.
 *
 * Usage:  LOAD_LEVEL=1 ./stability_bench_cpp
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

using namespace std::chrono;

// ─── Configuration ────────────────────────────────────────────────────────────

struct Config {
    size_t      payload_size;
    microseconds send_sleep;    // 0 = no sleep (safe in C++ without GIL issue)
    seconds     chaos_interval;
    seconds     duration;
};

Config get_config(int level) {
    switch (level) {
        case 1:  return {1024,   microseconds(1000), seconds(20), seconds(60)};
        case 2:  return {4096,   microseconds(100),  seconds(10), seconds(60)};
        case 3:  return {65536,  microseconds(0),    seconds(7),  seconds(60)};
        default: return {4096,   microseconds(0),    seconds(2),  seconds(180)};
    }
}

static constexpr uint16_t UNILINK_PORT = 10003;
static constexpr int      CHAOS_DOWN_S = 2;

// ─── Stats helper ─────────────────────────────────────────────────────────────

static void print_stats(const std::string& label, int level,
                        int reconnects,
                        uint64_t sent, uint64_t recv,
                        const std::vector<double>& snaps) {
    double avg = 0, sd = 0;
    if (!snaps.empty()) {
        avg = std::accumulate(snaps.begin(), snaps.end(), 0.0) / snaps.size();
        if (snaps.size() > 1) {
            double sq = 0;
            for (double v : snaps) sq += (v - avg) * (v - avg);
            sd = std::sqrt(sq / (snaps.size() - 1));
        }
    }
    double sent_mb = sent / (1024.0 * 1024.0);
    double recv_mb = recv / (1024.0 * 1024.0);
    double delivery = sent > 0 ? 100.0 * recv_mb / sent_mb : 0.0;

    std::cout << "\n--- " << label << " Results (Level " << level << ") ---\n"
              << "Reconnect:         " << reconnects    << "\n"
              << "Sent:              " << sent_mb       << " MB\n"
              << "Server Received:   " << recv_mb       << " MB\n"
              << "Delivery Rate:     " << delivery      << "%\n"
              << "Throughput Avg:    " << avg           << " MB/s\n"
              << "Throughput StdDev: " << sd            << " MB/s\n"
              << "--------------------------------------------\n";
}

// ─── Unilink C++ Benchmark ────────────────────────────────────────────────────

void run_unilink(int level, const Config& cfg) {
    std::cout << "--- Unilink C++ Stability Bench (Level " << level << ") ---\n"
              << "Payload: " << cfg.payload_size << " bytes"
              << "  send_sleep: " << cfg.send_sleep.count() << " us"
              << "  chaos: " << cfg.chaos_interval.count() << "s\n";

    std::atomic<uint64_t> sent_bytes{0};
    std::atomic<uint64_t> recv_bytes{0};
    std::atomic<int>      reconnects{0};
    std::atomic<bool>     send_allowed{true};
    std::atomic<bool>     running{true};

    std::mutex             snap_mu;
    std::vector<double>    snaps;

    // Server factory — recreated on each start
    std::mutex                         srv_mu;
    std::unique_ptr<unilink::TcpServer> server;

    auto start_server = [&] {
        std::lock_guard<std::mutex> lk(srv_mu);
        server = std::make_unique<unilink::TcpServer>(UNILINK_PORT);
        server->on_data([&](const unilink::MessageContext& ctx) {
            recv_bytes.fetch_add(ctx.data().size(), std::memory_order_relaxed);
        });
        server->start_sync();
    };

    auto stop_server = [&] {
        std::lock_guard<std::mutex> lk(srv_mu);
        if (server) { server->stop(); server.reset(); }
    };

    start_server();

    unilink::TcpClient client("127.0.0.1", UNILINK_PORT);
    client.on_connect([&](const unilink::ConnectionContext&) {
        reconnects.fetch_add(1, std::memory_order_relaxed);
    });
    client.on_backpressure([&](size_t queued) {
        // Mirror Python on_bp: ON when >= bp_high_ (16 MiB), OFF when <= bp_low_ (8 MiB)
        if (queued > 8u * 1024u * 1024u)
            send_allowed.store(false, std::memory_order_relaxed);
        else
            send_allowed.store(true,  std::memory_order_relaxed);
    });
    if (!client.start_sync()) {
        std::cerr << "Failed to connect\n";
        return;
    }

    // Sender thread
    std::thread sender([&] {
        std::string payload(cfg.payload_size, 'A');
        while (running.load(std::memory_order_relaxed)) {
            if (!send_allowed.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(microseconds(100));
                continue;
            }
            if (client.connected()) {
                if (client.send(payload))
                    sent_bytes.fetch_add(cfg.payload_size, std::memory_order_relaxed);
                if (cfg.send_sleep.count() > 0)
                    std::this_thread::sleep_for(cfg.send_sleep);
            } else {
                send_allowed.store(true, std::memory_order_relaxed);
                std::this_thread::sleep_for(milliseconds(10));
            }
        }
    });

    // Chaos thread
    std::thread chaos([&] {
        auto deadline = steady_clock::now() + cfg.duration;
        while (running.load() && steady_clock::now() < deadline) {
            std::this_thread::sleep_for(cfg.chaos_interval);
            if (!running.load()) break;
            stop_server();
            std::this_thread::sleep_for(seconds(CHAOS_DOWN_S));
            if (!running.load()) break;
            start_server();
        }
    });

    // Monitor thread
    std::thread monitor([&] {
        auto deadline = steady_clock::now() + cfg.duration;
        while (running.load() && steady_clock::now() < deadline) {
            uint64_t prev = sent_bytes.load();
            std::this_thread::sleep_for(seconds(1));
            uint64_t curr = sent_bytes.load();
            double tp = (curr - prev) / (1024.0 * 1024.0);
            std::lock_guard<std::mutex> lk(snap_mu);
            snaps.push_back(tp);
        }
    });

    // Main wait loop
    auto deadline = steady_clock::now() + cfg.duration;
    int elapsed = 0;
    while (steady_clock::now() < deadline) {
        std::this_thread::sleep_for(seconds(10));
        elapsed += 10;
        std::cout << "[Heartbeat] " << elapsed << "s elapsed\n";
    }

    running.store(false);
    send_allowed.store(true);  // unblock sender
    sender.join();
    chaos.join();
    monitor.join();

    stop_server();
    client.stop();

    print_stats("Unilink C++", level,
                reconnects.load(), sent_bytes.load(), recv_bytes.load(), snaps);
}

// ─── Raw Socket C++ Benchmark ─────────────────────────────────────────────────

static constexpr uint16_t RAW_PORT = 10004;
// Mirror Unilink retry: 100ms fast first, 1000ms subsequent
static constexpr int RETRY_FIRST_MS    = 100;
static constexpr int RETRY_INTERVAL_MS = 1000;

void run_raw(int level, const Config& cfg) {
    std::cout << "--- Raw Socket C++ Stability Bench (Level " << level << ") ---\n"
              << "Payload: " << cfg.payload_size << " bytes"
              << "  send_sleep: " << cfg.send_sleep.count() << " us"
              << "  chaos: " << cfg.chaos_interval.count() << "s\n";

    signal(SIGPIPE, SIG_IGN);  // avoid crash on broken pipe

    std::atomic<uint64_t> sent_bytes{0};
    std::atomic<uint64_t> recv_bytes{0};
    std::atomic<int>      reconnects{0};
    std::atomic<int>      exceptions{0};
    std::atomic<bool>     server_active{false};
    std::atomic<bool>     running{true};

    std::vector<double>   snaps;
    std::mutex            snap_mu;

    // ── Server ──
    std::atomic<int> server_fd{-1};

    auto start_server = [&] {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(RAW_PORT);
        ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        ::listen(fd, 128);
        server_fd.store(fd);
        server_active.store(true);
    };

    auto stop_server = [&] {
        server_active.store(false);
        int fd = server_fd.exchange(-1);
        if (fd >= 0) ::close(fd);
    };

    // Server accept + drain loop
    std::thread server_t([&] {
        while (running.load()) {
            int lfd = server_fd.load();
            if (lfd < 0 || !server_active.load()) {
                std::this_thread::sleep_for(milliseconds(10));
                continue;
            }
            struct timeval tv{0, 100000};
            setsockopt(lfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            sockaddr_in cli{};
            socklen_t len = sizeof(cli);
            int cfd = ::accept(lfd, reinterpret_cast<sockaddr*>(&cli), &len);
            if (cfd < 0) continue;

            std::thread([&, cfd] {
                std::vector<char> buf(65536);
                while (server_active.load()) {
                    ssize_t n = ::recv(cfd, buf.data(), buf.size(), 0);
                    if (n <= 0) break;
                    recv_bytes.fetch_add(static_cast<uint64_t>(n), std::memory_order_relaxed);
                }
                ::close(cfd);
            }).detach();
        }
    });

    start_server();

    // Chaos thread
    std::thread chaos([&] {
        auto deadline = steady_clock::now() + cfg.duration;
        while (running.load() && steady_clock::now() < deadline) {
            std::this_thread::sleep_for(cfg.chaos_interval);
            if (!running.load()) break;
            stop_server();
            std::this_thread::sleep_for(seconds(CHAOS_DOWN_S));
            if (!running.load()) break;
            start_server();
        }
    });

    // Monitor thread
    std::thread monitor([&] {
        auto deadline = steady_clock::now() + cfg.duration;
        while (running.load() && steady_clock::now() < deadline) {
            uint64_t prev = sent_bytes.load();
            std::this_thread::sleep_for(seconds(1));
            uint64_t curr = sent_bytes.load();
            double tp = (curr - prev) / (1024.0 * 1024.0);
            std::lock_guard<std::mutex> lk(snap_mu);
            snaps.push_back(tp);
        }
    });

    // Sender thread
    std::thread sender([&] {
        std::string payload(cfg.payload_size, 'A');
        int retry_attempt = 0;
        int sock = -1;

        while (running.load()) {
            if (sock < 0) {
                // Reconnect with fast-first retry
                sock = ::socket(AF_INET, SOCK_STREAM, 0);
                struct timeval tv{1, 0};
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
                addr.sin_port = htons(RAW_PORT);
                if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                    reconnects.fetch_add(1, std::memory_order_relaxed);
                    retry_attempt = 0;
                } else {
                    ::close(sock);
                    sock = -1;
                    int wait_ms = (retry_attempt == 0) ? RETRY_FIRST_MS : RETRY_INTERVAL_MS;
                    retry_attempt++;
                    std::this_thread::sleep_for(milliseconds(wait_ms));
                    continue;
                }
            }

            ssize_t n = ::send(sock, payload.data(), payload.size(), MSG_NOSIGNAL);
            if (n > 0) {
                sent_bytes.fetch_add(static_cast<uint64_t>(n), std::memory_order_relaxed);
                if (cfg.send_sleep.count() > 0)
                    std::this_thread::sleep_for(cfg.send_sleep);
            } else {
                exceptions.fetch_add(1, std::memory_order_relaxed);
                ::close(sock);
                sock = -1;
                retry_attempt = 0;
            }
        }
        if (sock >= 0) ::close(sock);
    });

    // Main wait
    auto deadline = steady_clock::now() + cfg.duration;
    int elapsed = 0;
    while (steady_clock::now() < deadline) {
        std::this_thread::sleep_for(seconds(10));
        elapsed += 10;
        std::cout << "[Heartbeat] " << elapsed << "s elapsed\n";
    }

    running.store(false);
    stop_server();
    sender.join();
    chaos.join();
    monitor.join();
    server_t.join();

    std::cout << "Exceptions: " << exceptions.load() << "\n";
    print_stats("Raw Socket C++", level,
                reconnects.load(), sent_bytes.load(), recv_bytes.load(), snaps);
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    int level = 1;
    const char* mode = "unilink";

    const char* env_level = std::getenv("LOAD_LEVEL");
    const char* env_mode  = std::getenv("MODE");
    if (env_level) level = std::atoi(env_level);
    if (env_mode)  mode  = env_mode;
    if (argc > 1) level = std::atoi(argv[1]);
    if (argc > 2) mode  = argv[2];

    Config cfg = get_config(level);

    if (std::string(mode) == "raw")
        run_raw(level, cfg);
    else
        run_unilink(level, cfg);

    return 0;
}
