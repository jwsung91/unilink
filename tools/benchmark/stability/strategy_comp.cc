#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <iomanip>
#include "unilink/unilink.hpp"

using namespace std::chrono;

struct BenchResult {
    double sent_mb;
    double recv_mb;
    double throughput_mb_s;
    double delivery_rate;
};

BenchResult run_bench(unilink::base::constants::BackpressureStrategy strategy, int duration_s) {
    const uint16_t port = (strategy == unilink::base::constants::BackpressureStrategy::Reliable) ? 10020 : 10021;
    unilink::TcpServer server(port);
    std::atomic<uint64_t> recv_bytes{0};
    
    server.on_data([&](const unilink::MessageContext& ctx) {
        recv_bytes.fetch_add(ctx.data().size(), std::memory_order_relaxed);
        // Simulate slow consumer: 1ms delay
        std::this_thread::sleep_for(milliseconds(1));
    });
    server.start_sync();

    unilink::TcpClient client("127.0.0.1", port);
    client.backpressure_threshold(1024 * 1024); // 1MB threshold
    client.backpressure_strategy(strategy);
    client.start_sync();

    std::atomic<bool> running{true};
    std::atomic<uint64_t> sent_bytes{0};
    std::string payload(1024, 'A'); // 1KB messages

    std::thread sender([&] {
        while (running) {
            if (client.send(payload)) {
                sent_bytes.fetch_add(payload.size(), std::memory_order_relaxed);
            } else {
                // BestEffort might return false if disconnected, but Reliable blocks.
                if (strategy == unilink::base::constants::BackpressureStrategy::BestEffort) {
                    // Small sleep to prevent tight loop if queue is persistently full (though BestEffort usually drops)
                    std::this_thread::sleep_for(microseconds(10));
                }
            }
        }
    });

    auto start = steady_clock::now();
    std::this_thread::sleep_for(seconds(duration_s));
    running = false;
    sender.join();
    
    auto end = steady_clock::now();
    double elapsed = duration_cast<duration<double>>(end - start).count();
    
    server.stop();
    client.stop();

    double s_mb = sent_bytes / (1024.0*1024.0);
    double r_mb = recv_bytes / (1024.0*1024.0);
    
    return {s_mb, r_mb, s_mb / elapsed, (s_mb > 0 ? (r_mb / s_mb * 100.0) : 0.0)};
}

int main() {
    std::cout << "Starting Strategy Comparison (Slow Consumer: 1ms delay/msg, Duration: 5s)...\n" << std::endl;
    
    auto reliable = run_bench(unilink::base::constants::BackpressureStrategy::Reliable, 5);
    auto best_effort = run_bench(unilink::base::constants::BackpressureStrategy::BestEffort, 5);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::left << std::setw(20) << "Metric" 
              << std::setw(20) << "Reliable" 
              << std::setw(20) << "BestEffort" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    std::cout << std::setw(20) << "Sent (MB)" 
              << std::setw(20) << reliable.sent_mb 
              << std::setw(20) << best_effort.sent_mb << std::endl;
              
    std::cout << std::setw(20) << "Received (MB)" 
              << std::setw(20) << reliable.recv_mb 
              << std::setw(20) << best_effort.recv_mb << std::endl;

    std::cout << std::setw(20) << "Throughput (MB/s)" 
              << std::setw(20) << reliable.throughput_mb_s 
              << std::setw(20) << best_effort.throughput_mb_s << std::endl;

    std::cout << std::setw(20) << "Delivery Rate (%)" 
              << std::setw(20) << reliable.delivery_rate 
              << std::setw(20) << best_effort.delivery_rate << std::endl;

    return 0;
}
