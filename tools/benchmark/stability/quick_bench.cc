#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include "unilink/unilink.hpp"

using namespace std::chrono;

void run_unilink(int duration_s) {
    unilink::TcpServer server(10010);
    std::atomic<uint64_t> recv_bytes{0};
    server.on_data([&](const unilink::MessageContext& ctx) {
        recv_bytes.fetch_add(ctx.data().size(), std::memory_order_relaxed);
    });
    server.start_sync();

    unilink::TcpClient client("127.0.0.1", 10010);
    client.backpressure_strategy(unilink::base::constants::BackpressureStrategy::Reliable);
    client.start_sync();

    std::atomic<bool> running{true};
    std::atomic<uint64_t> sent_bytes{0};
    std::string payload(65536, 'A');

    std::thread sender([&] {
        while (running) {
            if (client.send(payload)) {
                sent_bytes += payload.size();
            } else {
                std::this_thread::sleep_for(microseconds(10));
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

    std::cout << "--- Unilink (Reliable) ---\n"
              << "Sent: " << sent_bytes / (1024.0*1024.0) << " MB\n"
              << "Recv: " << recv_bytes / (1024.0*1024.0) << " MB\n"
              << "Throughput: " << (sent_bytes / (1024.0*1024.0)) / elapsed << " MB/s\n";
}

int main() {
    run_unilink(10);
    return 0;
}
