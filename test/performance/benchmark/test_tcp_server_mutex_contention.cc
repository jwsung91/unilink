/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "test/utils/test_utils.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/tcp_server/boost_tcp_acceptor.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

using namespace unilink;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpServerMutexContentionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Get a unique available port to avoid conflicts
    test_port_ = test::TestUtils::getAvailableTestPort();

    // Config setup
    config::TcpServerConfig cfg;
    cfg.port = test_port_;
    cfg.max_connections = 1000;
    cfg.backpressure_threshold = 1024 * 1024;

    // Create server with 2 threads (reduced for CI stability)
    acceptor_ = std::make_unique<transport::BoostTcpAcceptor>(ioc_);
    server_ = transport::TcpServer::create(cfg, std::move(acceptor_), ioc_);
  }

  void TearDown() override {
    if (server_) server_->stop();
    ioc_.stop();
    for (auto& t : threads_) {
      if (t.joinable()) t.join();
    }
  }

  uint16_t test_port_;
  net::io_context ioc_;
  std::unique_ptr<transport::BoostTcpAcceptor> acceptor_;  // Temporarily held until moved
  std::shared_ptr<transport::TcpServer> server_;
  std::vector<std::thread> threads_;
};

TEST_F(TcpServerMutexContentionTest, BenchmarkThroughput) {
  server_->start();

  // Start IO threads
  int thread_count = 2;  // Reduced for CI stability
  for (int i = 0; i < thread_count; ++i) {
    threads_.emplace_back([this] { ioc_.run(); });
  }

  // Setup atomic counter
  std::atomic<size_t> bytes_received{0};
  server_->on_bytes(
      [&](memory::ConstByteSpan data) { bytes_received.fetch_add(data.size(), std::memory_order_relaxed); });

  // Clients
  const int client_count = 10;   // Reduced for CI stability
  const int duration_ms = 1000;  // Reduced for CI stability
  const size_t packet_size = 128;
  std::vector<uint8_t> packet(packet_size, 'X');

  std::vector<std::thread> client_threads;
  std::atomic<bool> running{true};
  std::atomic<size_t> total_sent{0};

  // Concurrent Status Reader Thread (simulating contention on shared_mutex)
  // This thread repeatedly calls get_client_count() which uses std::shared_lock
  // while clients are connecting/disconnecting (which uses std::unique_lock)
  // and sending data (which uses std::shared_lock for callback lookup).
  std::thread status_reader([&] {
    while (running.load(std::memory_order_relaxed)) {
      volatile size_t count = server_->get_client_count();
      (void)count;
      std::this_thread::yield();
    }
  });

  for (int i = 0; i < client_count; ++i) {
    client_threads.emplace_back([&] {
      try {
        net::io_context client_ioc;
        tcp::socket socket(client_ioc);
        tcp::endpoint ep(net::ip::make_address("127.0.0.1"), test_port_);

        boost::system::error_code ec;
        // Retry connection
        for (int k = 0; k < 20; ++k) {
          socket.connect(ep, ec);
          if (!ec) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (ec) return;

        while (running.load(std::memory_order_relaxed)) {
          boost::system::error_code wec;
          net::write(socket, net::buffer(packet), wec);
          if (wec) break;
          total_sent.fetch_add(packet_size, std::memory_order_relaxed);
        }
      } catch (...) {
      }
    });
  }

  std::cout << "Benchmarking with " << client_count << " clients for " << duration_ms << "ms on port " << test_port_
            << "..." << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
  running = false;

  for (auto& t : client_threads) {
    if (t.joinable()) t.join();
  }

  if (status_reader.joinable()) status_reader.join();

  // Wait a bit for server to process remaining data
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  double seconds = duration_ms / 1000.0;
  double throughput_mb = (static_cast<double>(bytes_received.load()) / 1024.0 / 1024.0) / seconds;
  double throughput_ops = (static_cast<double>(bytes_received.load()) / packet_size) / seconds;

  std::cout << "Results:" << std::endl;
  std::cout << "  Bytes Received: " << bytes_received.load() << std::endl;
  std::cout << "  Throughput: " << throughput_mb << " MB/s" << std::endl;
  std::cout << "  Ops/sec: " << throughput_ops << std::endl;
}
