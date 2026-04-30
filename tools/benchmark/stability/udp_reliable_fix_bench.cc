#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

using namespace std::chrono;

struct BenchResult {
  double tp;
  double dr;
};

BenchResult run_udp_fixed(size_t msg_size, int duration_s) {
  static uint16_t base_port = 16000;
  uint16_t ps = base_port++;
  uint16_t pc = base_port++;

  // Server with large enough buffer for any test packet
  unilink::TcpServer dummy(0);  // Just to ensure system is ready

  unilink::UdpServer server(ps);
  std::atomic<uint64_t> rb{0};
  server.on_data([&](const unilink::MessageContext& ctx) { rb += ctx.data().size(); });
  server.start_sync();

  unilink::config::UdpConfig cfg;
  cfg.local_port = pc;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = ps;

  unilink::UdpClient client(cfg);
  client.backpressure_strategy(unilink::base::constants::BackpressureStrategy::Reliable);
  client.start_sync();

  std::atomic<bool> run{true};
  std::atomic<uint64_t> sb{0};
  std::string p(msg_size, 'A');

  std::thread sender([&] {
    while (run) {
      if (client.send(p)) sb += p.size();
      // Minor yield to prevent kernel buffer saturation during loopback
      std::this_thread::sleep_for(microseconds(10));
    }
  });

  std::this_thread::sleep_for(seconds(duration_s));
  run = false;
  sender.join();
  std::this_thread::sleep_for(milliseconds(200));
  server.stop();
  client.stop();

  double sent_mb = sb / (1024.0 * 1024.0);
  double recv_mb = rb / (1024.0 * 1024.0);
  return {sent_mb / duration_s, (sent_mb > 0 ? (100.0 * recv_mb / sent_mb) : 0.0)};
}

int main() {
  std::cout << "UDP Reliable Consistency Check (Varying Message Sizes)" << std::endl;
  std::cout << "| Msg Size | Throughput (MB/s) | Delivery (%) |" << std::endl;
  std::cout << "|----------|-------------------|--------------|" << std::endl;

  size_t sizes[] = {1024, 1400, 4000, 8000};
  for (size_t s : sizes) {
    auto r = run_udp_fixed(s, 5);
    std::cout << "| " << std::setw(8) << s << " | " << std::fixed << std::setprecision(2) << std::setw(17) << r.tp
              << " | " << std::setw(12) << r.dr << " |" << std::endl;
  }
  return 0;
}
