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

struct BenchResult {
  double throughput_mb_s;
  double delivery_rate;
};

// TCP Bench
BenchResult test_tcp(unilink::base::constants::BackpressureStrategy strategy) {
  uint16_t port = 10220 + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 1);
  unilink::TcpServer server(port);
  std::atomic<uint64_t> recv_bytes{0};
  server.on_data([&](const unilink::MessageContext& ctx) { recv_bytes += ctx.data().size(); });
  (void)server.start_sync();
  unilink::TcpClient client("127.0.0.1", port);
  client.backpressure_strategy(strategy);
  (void)client.start_sync();

  std::atomic<bool> running{true};
  std::atomic<uint64_t> sent_bytes{0};
  std::string payload(32768, 'A');
  std::thread sender([&] {
    while (running) {
      if (client.send(payload)) sent_bytes += payload.size();
    }
  });
  std::this_thread::sleep_for(seconds(3));
  running = false;
  sender.join();
  server.stop();
  client.stop();
  return {(sent_bytes / (1024.0 * 1024.0)) / 3.0, (sent_bytes > 0 ? (100.0 * recv_bytes / sent_bytes) : 0.0)};
}

// UDS Bench
BenchResult test_uds(unilink::base::constants::BackpressureStrategy strategy) {
  std::string path = "/tmp/unilink_test_" + std::to_string(static_cast<int>(strategy)) + ".sock";
  fs::remove(path);
  unilink::UdsServer server(path);
  std::atomic<uint64_t> recv_bytes{0};
  server.on_data([&](const unilink::MessageContext& ctx) { recv_bytes += ctx.data().size(); });
  (void)server.start_sync();
  unilink::UdsClient client(path);
  client.backpressure_strategy(strategy);
  (void)client.start_sync();

  std::atomic<bool> running{true};
  std::atomic<uint64_t> sent_bytes{0};
  std::string payload(32768, 'A');
  std::thread sender([&] {
    while (running) {
      if (client.send(payload)) sent_bytes += payload.size();
    }
  });
  std::this_thread::sleep_for(seconds(3));
  running = false;
  sender.join();
  server.stop();
  client.stop();
  fs::remove(path);
  return {(sent_bytes / (1024.0 * 1024.0)) / 3.0, (sent_bytes > 0 ? (100.0 * recv_bytes / sent_bytes) : 0.0)};
}

// UDP Bench
BenchResult test_udp(unilink::base::constants::BackpressureStrategy strategy) {
  uint16_t port_s = 10320 + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 1);
  uint16_t port_c = 10420 + (strategy == unilink::base::constants::BackpressureStrategy::Reliable ? 0 : 1);

  unilink::UdpServer server(port_s);
  std::atomic<uint64_t> recv_bytes{0};
  server.on_data([&](const unilink::MessageContext& ctx) { recv_bytes += ctx.data().size(); });
  (void)server.start_sync();

  unilink::config::UdpConfig cfg;
  cfg.local_port = port_c;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = port_s;
  unilink::UdpClient client(cfg);
  client.backpressure_strategy(strategy);
  (void)client.start_sync();

  std::atomic<bool> running{true};
  std::atomic<uint64_t> sent_bytes{0};
  std::string payload(1024, 'A');
  std::thread sender([&] {
    while (running) {
      if (client.send(payload)) sent_bytes += payload.size();
    }
  });
  std::this_thread::sleep_for(seconds(3));
  running = false;
  sender.join();
  server.stop();
  client.stop();
  return {(sent_bytes / (1024.0 * 1024.0)) / 3.0, (sent_bytes > 0 ? (100.0 * recv_bytes / sent_bytes) : 0.0)};
}

int main() {
  std::cout << "| Proto | Reliable (MB/s) | Rel. Delivery | BestEffort (MB/s) | BE Delivery |" << std::endl;
  std::cout << "|-------|-----------------|---------------|-------------------|-------------|" << std::endl;

  auto tcp_r = test_tcp(unilink::base::constants::BackpressureStrategy::Reliable);
  auto tcp_b = test_tcp(unilink::base::constants::BackpressureStrategy::BestEffort);
  std::cout << "| TCP   | " << std::fixed << std::setprecision(2) << std::setw(15) << tcp_r.throughput_mb_s << " | "
            << std::setw(12) << tcp_r.delivery_rate << "% | " << std::setw(17) << tcp_b.throughput_mb_s << " | "
            << std::setw(10) << tcp_b.delivery_rate << "% |" << std::endl;

  auto uds_r = test_uds(unilink::base::constants::BackpressureStrategy::Reliable);
  auto uds_b = test_uds(unilink::base::constants::BackpressureStrategy::BestEffort);
  std::cout << "| UDS   | " << std::fixed << std::setprecision(2) << std::setw(15) << uds_r.throughput_mb_s << " | "
            << std::setw(12) << uds_r.delivery_rate << "% | " << std::setw(17) << uds_b.throughput_mb_s << " | "
            << std::setw(10) << uds_b.delivery_rate << "% |" << std::endl;

  auto udp_r = test_udp(unilink::base::constants::BackpressureStrategy::Reliable);
  auto udp_b = test_udp(unilink::base::constants::BackpressureStrategy::BestEffort);
  std::cout << "| UDP   | " << std::fixed << std::setprecision(2) << std::setw(15) << udp_r.throughput_mb_s << " | "
            << std::setw(12) << udp_r.delivery_rate << "% | " << std::setw(17) << udp_b.throughput_mb_s << " | "
            << std::setw(10) << udp_b.delivery_rate << "% |" << std::endl;

  return 0;
}
