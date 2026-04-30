#include <atomic>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

using namespace std::chrono;
namespace fs = std::filesystem;

struct LoadParam {
  size_t payload_size;
  microseconds sleep_us;
};
LoadParam get_load(int level) {
  if (level == 1) return {1024, microseconds(1000)};
  if (level == 2) return {4096, microseconds(100)};
  if (level == 3) return {65536, microseconds(0)};
  return {4096, microseconds(0)};
}

struct BenchResult {
  double tp;
  double dr;
};

// --- TCP ---
BenchResult run_tcp(int level, unilink::base::constants::BackpressureStrategy strategy) {
  auto load = get_load(level);
  uint16_t port = 11000 + level + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 10);
  unilink::TcpServer server(port);
  std::atomic<uint64_t> rb{0};
  server.on_data([&](const unilink::MessageContext& ctx) { rb += ctx.data().size(); });
  (void)server.start_sync();
  unilink::TcpClient client("127.0.0.1", port);
  client.backpressure_strategy(strategy);
  (void)client.start_sync();
  std::atomic<bool> run{true};
  std::atomic<uint64_t> sb{0};
  std::string p(load.payload_size, 'A');
  std::thread sender([&] {
    while (run) {
      if (client.send(p)) sb += p.size();
      if (load.sleep_us.count() > 0) std::this_thread::sleep_for(load.sleep_us);
    }
  });
  std::this_thread::sleep_for(seconds(3));
  run = false;
  sender.join();
  server.stop();
  client.stop();
  return {(sb / (1024.0 * 1024.0)) / 3.0, (sb > 0 ? (100.0 * rb / sb) : 0.0)};
}

// --- UDS ---
BenchResult run_uds(int level, unilink::base::constants::BackpressureStrategy strategy) {
  auto load = get_load(level);
  std::string path =
      "/tmp/full_uds_" + std::to_string(level) + "_" + std::to_string(static_cast<int>(strategy)) + ".sock";
  fs::remove(path);
  unilink::UdsServer server(path);
  std::atomic<uint64_t> rb{0};
  server.on_data([&](const unilink::MessageContext& ctx) { rb += ctx.data().size(); });
  (void)server.start_sync();
  unilink::UdsClient client(path);
  client.backpressure_strategy(strategy);
  (void)client.start_sync();
  std::atomic<bool> run{true};
  std::atomic<uint64_t> sb{0};
  std::string p(load.payload_size, 'A');
  std::thread sender([&] {
    while (run) {
      if (client.send(p)) sb += p.size();
      if (load.sleep_us.count() > 0) std::this_thread::sleep_for(load.sleep_us);
    }
  });
  std::this_thread::sleep_for(seconds(3));
  run = false;
  sender.join();
  server.stop();
  client.stop();
  fs::remove(path);
  return {(sb / (1024.0 * 1024.0)) / 3.0, (sb > 0 ? (100.0 * rb / sb) : 0.0)};
}

// --- UDP ---
BenchResult run_udp(int level, unilink::base::constants::BackpressureStrategy strategy) {
  auto load = get_load(level);
  uint16_t ps = 12000 + level + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 10);
  uint16_t pc = 13000 + level + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 10);
  unilink::UdpServer server(ps);
  std::atomic<uint64_t> rb{0};
  server.on_data([&](const unilink::MessageContext& ctx) { rb += ctx.data().size(); });
  (void)server.start_sync();
  unilink::config::UdpConfig cfg;
  cfg.local_port = pc;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = ps;
  unilink::UdpClient client(cfg);
  client.backpressure_strategy(strategy);
  (void)client.start_sync();
  std::atomic<bool> run{true};
  std::atomic<uint64_t> sb{0};
  // UDP: Keep payload within MTU safely for stress levels
  size_t sz = (level >= 3) ? 1400 : load.payload_size;
  std::string p(sz, 'A');
  std::thread sender([&] {
    while (run) {
      if (client.send(p)) sb += p.size();
      if (load.sleep_us.count() > 0) std::this_thread::sleep_for(load.sleep_us);
    }
  });
  std::this_thread::sleep_for(seconds(3));
  run = false;
  sender.join();
  server.stop();
  client.stop();
  return {(sb / (1024.0 * 1024.0)) / 3.0, (sb > 0 ? (100.0 * rb / sb) : 0.0)};
}

int main() {
  std::cout << "| Proto | Level | Strategy | Throughput (MB/s) | Delivery (%) |" << std::endl;
  std::cout << "|-------|-------|----------|-------------------|--------------|" << std::endl;
  auto run_all = [](std::string name, auto func) {
    for (int l = 1; l <= 4; ++l) {
      auto r = func(l, unilink::base::constants::BackpressureStrategy::Reliable);
      std::cout << "| " << std::left << std::setw(5) << name << " | L" << l << " | Reliable | " << std::fixed
                << std::setprecision(2) << std::setw(17) << r.tp << " | " << std::setw(12) << r.dr << " |" << std::endl;
      auto b = func(l, unilink::base::constants::BackpressureStrategy::BestEffort);
      std::cout << "| " << std::left << std::setw(5) << name << " | L" << l << " | BestEffort | " << std::fixed
                << std::setprecision(2) << std::setw(15) << b.tp << " | " << std::setw(12) << b.dr << " |" << std::endl;
    }
  };
  run_all("TCP", run_tcp);
  run_all("UDS", run_uds);
  run_all("UDP", run_udp);
  return 0;
}
