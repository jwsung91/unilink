#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include "unilink/unilink.hpp"

using namespace std::chrono;

struct LoadParam {
    size_t payload_size;
    microseconds sleep_us;
};

struct BenchResult {
    double throughput_mb_s;
    double delivery_rate;
};

LoadParam get_load(int level) {
    if (level == 1) return {1024, microseconds(1000)};
    if (level == 2) return {4096, microseconds(100)};
    if (level == 3) return {65536, microseconds(0)};
    return {4096, microseconds(0)};
}

BenchResult run_single_bench(int level, unilink::base::constants::BackpressureStrategy strategy) {
    auto load = get_load(level);
    uint16_t port = 10100 + level + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 10);
    
    unilink::TcpServer server(port);
    std::atomic<uint64_t> recv_bytes{0};
    server.on_data([&](const unilink::MessageContext& ctx) {
        recv_bytes.fetch_add(ctx.data().size(), std::memory_order_relaxed);
    });
    server.start_sync();

    unilink::TcpClient client("127.0.0.1", port);
    client.backpressure_threshold(4 * 1024 * 1024); // 4MB for testing
    client.backpressure_strategy(strategy);
    client.start_sync();

    std::atomic<bool> running{true};
    std::atomic<uint64_t> sent_bytes{0};
    std::string payload(load.payload_size, 'A');

    std::thread sender([&] {
        while (running) {
            if (client.send(payload)) {
                sent_bytes.fetch_add(payload.size(), std::memory_order_relaxed);
            }
            if (load.sleep_us.count() > 0) std::this_thread::sleep_for(load.sleep_us);
        }
    });

    auto start = steady_clock::now();
    std::this_thread::sleep_for(seconds(5));
    running = false;
    sender.join();
    
    auto end = steady_clock::now();
    double elapsed = duration_cast<duration<double>>(end - start).count();
    
    server.stop();
    client.stop();

    double s_mb = sent_bytes / (1024.0*1024.0);
    double r_mb = recv_bytes / (1024.0*1024.0);
    
    return {s_mb / elapsed, (s_mb > 0 ? (r_mb / s_mb * 100.0) : 0.0)};
}

int main() {
    std::cout << "| Level | Strategy | Throughput (MB/s) | Delivery (%) |" << std::endl;
    std::cout << "|-------|----------|-------------------|--------------|" << std::endl;
    
    for (int l = 1; l <= 4; ++l) {
        auto res_r = run_single_bench(l, unilink::base::constants::BackpressureStrategy::Reliable);
        std::cout << "| L" << l << " | Reliable | " << std::fixed << std::setprecision(2) 
                  << std::setw(17) << res_r.throughput_mb_s << " | " << std::setw(12) << res_r.delivery_rate << " |" << std::endl;
        
        auto res_b = run_single_bench(l, unilink::base::constants::BackpressureStrategy::BestEffort);
        std::cout << "| L" << l << " | BestEffort | " << std::fixed << std::setprecision(2) 
                  << std::setw(15) << res_b.throughput_mb_s << " | " << std::setw(12) << res_b.delivery_rate << " |" << std::endl;
    }
    return 0;
}
