#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono;

const std::string PING_MESSAGE = "PING";
const size_t CHUNK_SIZE = 1024;
const std::string CHUNK_MESSAGE(CHUNK_SIZE, 'A');

template <typename T>
double run_pingpong(T& master, T& slave, int num_pings) {
  std::atomic<int> completed{0};
  std::condition_variable cv;
  std::mutex mtx;

  slave.on_data([&](const auto& ctx) {
    if (!ctx.data().empty()) {
      slave.send(std::string_view(ctx.data().data(), ctx.data().size()));
    }
  });

  master.on_data([&](const auto& ctx) {
    if (!ctx.data().empty()) {
      if (++completed < num_pings) {
        master.send(PING_MESSAGE);
      } else {
        cv.notify_one();
      }
    }
  });

  master.start().get();
  slave.start().get();
  std::this_thread::sleep_for(milliseconds(200));

  auto start = high_resolution_clock::now();
  master.send(PING_MESSAGE);

  std::unique_lock<std::mutex> lock(mtx);
  if (!cv.wait_for(lock, seconds(10), [&] { return completed >= num_pings; })) {
    master.stop();
    slave.stop();
    return -1.0;
  }
  auto end = high_resolution_clock::now();

  master.stop();
  slave.stop();
  return duration<double>(end - start).count();
}

double run_tcp_pingpong(int num_pings) {
  TcpServer server(9991);
  TcpClient client("127.0.0.1", 9991);
  std::atomic<int> completed{0};
  std::condition_variable cv;
  std::mutex mtx;

  server.on_data([&](const auto& ctx) {
    if (!ctx.data().empty()) {
      server.send_to(ctx.client_id(), std::string_view(ctx.data().data(), ctx.data().size()));
    }
  });

  client.on_data([&](const auto& ctx) {
    if (!ctx.data().empty()) {
      if (++completed < num_pings) {
        client.send(PING_MESSAGE);
      } else {
        cv.notify_one();
      }
    }
  });

  server.start().get();
  client.start().get();
  std::this_thread::sleep_for(milliseconds(200));

  auto start = high_resolution_clock::now();
  client.send(PING_MESSAGE);

  std::unique_lock<std::mutex> lock(mtx);
  if (!cv.wait_for(lock, seconds(10), [&] { return completed >= num_pings; })) {
    return -1.0;
  }
  auto end = high_resolution_clock::now();

  client.stop();
  server.stop();
  return duration<double>(end - start).count();
}

double run_tcp_throughput(int num_chunks) {
  TcpServer server(9992);
  TcpClient client("127.0.0.1", 9992);
  size_t total_bytes = num_chunks * CHUNK_SIZE;
  std::atomic<size_t> received_bytes{0};
  std::condition_variable cv;
  std::mutex mtx;

  server.on_data([&](const auto& ctx) {
    received_bytes += ctx.data().size();
    if (received_bytes >= total_bytes) cv.notify_one();
  });

  server.start().get();
  client.start().get();
  std::this_thread::sleep_for(milliseconds(200));

  auto start = high_resolution_clock::now();
  for (int i = 0; i < num_chunks; ++i) {
    client.send(CHUNK_MESSAGE);
  }

  std::unique_lock<std::mutex> lock(mtx);
  cv.wait_for(lock, seconds(10), [&] { return received_bytes >= total_bytes; });
  auto end = high_resolution_clock::now();

  client.stop();
  server.stop();
  return duration<double>(end - start).count();
}

double run_uds_pingpong(int num_pings) {
  std::string path = "/tmp/cpp_bench.sock";
  std::remove(path.c_str());
  UdsServer server(path);
  UdsClient client(path);
  std::atomic<int> completed{0};
  std::condition_variable cv;
  std::mutex mtx;

  server.on_data([&](const auto& ctx) {
    if (!ctx.data().empty()) server.send_to(ctx.client_id(), std::string_view(ctx.data().data(), ctx.data().size()));
  });
  client.on_data([&](const auto& ctx) {
    if (!ctx.data().empty()) {
      if (++completed < num_pings)
        client.send(PING_MESSAGE);
      else
        cv.notify_one();
    }
  });

  server.start().get();
  client.start().get();
  std::this_thread::sleep_for(milliseconds(200));

  auto start = high_resolution_clock::now();
  client.send(PING_MESSAGE);

  std::unique_lock<std::mutex> lock(mtx);
  if (!cv.wait_for(lock, seconds(10), [&] { return completed >= num_pings; })) {
    client.stop();
    server.stop();
    return -1.0;
  }
  auto end = high_resolution_clock::now();

  client.stop();
  server.stop();
  return duration<double>(end - start).count();
}

int main() {
  std::cout << "=== C++ Core Benchmark (Optimized) ===" << std::endl;
  std::cout << std::fixed << std::setprecision(4);

  std::cout << "\n--- TCP ---" << std::endl;
  std::cout << "1000 Pings: " << run_tcp_pingpong(1000) << " sec" << std::endl;
  std::cout << "1000 Chunks (1MB): " << run_tcp_throughput(1000) << " sec" << std::endl;

  // UDP
  {
    unilink::config::UdpConfig cfg1;
    cfg1.local_port = 9993;
    cfg1.remote_address = "127.0.0.1";
    cfg1.remote_port = 9994;
    unilink::config::UdpConfig cfg2;
    cfg2.local_port = 9994;
    cfg2.remote_address = "127.0.0.1";
    cfg2.remote_port = 9993;
    UdpClient udp1(cfg1), udp2(cfg2);
    std::cout << "\n--- UDP ---" << std::endl;
    std::cout << "1000 Pings: " << run_pingpong(udp1, udp2, 1000) << " sec" << std::endl;
  }

  std::cout << "\n--- UDS ---" << std::endl;
  std::cout << "1000 Pings: " << run_uds_pingpong(1000) << " sec" << std::endl;

  return 0;
}
