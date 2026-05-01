#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

using namespace std::chrono;

struct LoadParam {
  size_t payload_size;
  microseconds sleep_us;
};
LoadParam get_load(int level) {
  if (level == 1) return {1024, microseconds(1000)};
  if (level == 2) return {4096, microseconds(100)};
  if (level == 3) return {8192, microseconds(0)};  // UDP: 8KB as high load
  return {1024, microseconds(0)};                  // L4: 1KB high PPS
}

struct BenchResult {
  double tp;
  double dr;
};

BenchResult run_udp_eval(int level, unilink::base::constants::BackpressureStrategy strategy) {
  auto load = get_load(level);
  // Use unique ports for every run to avoid lingering socket issues
  static uint16_t base_port = 15000;
  uint16_t ps = base_port++;
  uint16_t pc = base_port++;

  unilink::UdpServer server(ps);
  std::atomic<uint64_t> rb{0};
  server.on_data([&](const unilink::MessageContext& ctx) { rb += ctx.data().size(); });
  if (!server.start_sync()) return {0, 0};

  unilink::config::UdpConfig cfg;
  cfg.local_port = pc;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = ps;
  // CRITICAL: Ensure we have enough OS buffer for UDP stress
  cfg.enable_memory_pool = true;

  unilink::UdpClient client(cfg);
  client.backpressure_threshold(512 * 1024);  // 512KB
  client.backpressure_strategy(strategy);
  if (!client.start_sync()) {
    server.stop();
    return {0, 0};
  }

  std::atomic<bool> run{true};
  std::atomic<uint64_t> sb{0};
  std::string p(load.payload_size, 'A');

  std::thread sender([&] {
    while (run) {
      if (client.send(p)) {
        sb.fetch_add(p.size(), std::memory_order_relaxed);
      } else if (strategy == unilink::base::constants::BackpressureStrategy::Reliable) {
        // Reliable blocks inside send(), if it returns false it means stopped
        break;
      }
      if (load.sleep_us.count() > 0) std::this_thread::sleep_for(load.sleep_us);
    }
  });

  auto start = steady_clock::now();
  std::this_thread::sleep_for(seconds(5));  // Increased to 5s for better stability
  run = false;
  sender.join();

  auto end = steady_clock::now();
  double elapsed = duration_cast<duration<double>>(end - start).count();

  // Give some time for in-flight packets
  std::this_thread::sleep_for(milliseconds(200));

  server.stop();
  client.stop();

  double sent_mb = sb / (1024.0 * 1024.0);
  double recv_mb = rb / (1024.0 * 1024.0);

  return {sent_mb / elapsed, (sent_mb > 0 ? (100.0 * recv_mb / sent_mb) : 0.0)};
}

int main() {
  std::cout << "UDP Deep Analysis Matrix (Reliable vs BestEffort)" << std::endl;
  std::cout << "| Level | Strategy | Throughput (MB/s) | Delivery (%) |" << std::endl;
  std::cout << "|-------|----------|-------------------|--------------|" << std::endl;

  for (int l = 1; l <= 4; ++l) {
    auto r = run_udp_eval(l, unilink::base::constants::BackpressureStrategy::Reliable);
    std::cout << "| L" << l << " | Reliable | " << std::fixed << std::setprecision(2) << std::setw(17) << r.tp << " | "
              << std::setw(12) << r.dr << " |" << std::endl;

    auto b = run_udp_eval(l, unilink::base::constants::BackpressureStrategy::BestEffort);
    std::cout << "| L" << l << " | BestEffort | " << std::fixed << std::setprecision(2) << std::setw(15) << b.tp
              << " | " << std::setw(12) << b.dr << " |" << std::endl;
  }
  return 0;
}
