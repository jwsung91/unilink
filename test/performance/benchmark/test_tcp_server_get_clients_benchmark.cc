#include <gtest/gtest.h>

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

class TcpServerGetClientsBenchmarkTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_port_ = test::TestUtils::getAvailableTestPort();

    config::TcpServerConfig cfg;
    cfg.port = test_port_;
    cfg.max_connections = 10000;

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
  std::unique_ptr<transport::BoostTcpAcceptor> acceptor_;
  std::shared_ptr<transport::TcpServer> server_;
  std::vector<std::thread> threads_;
};

TEST_F(TcpServerGetClientsBenchmarkTest, BenchmarkGetClients) {
  server_->start();

  // Start IO thread
  threads_.emplace_back([this] { ioc_.run(); });

  const int client_count = 100;

  std::vector<std::shared_ptr<tcp::socket>> client_sockets;
  std::vector<std::thread> client_threads;

  for (int i = 0; i < client_count; ++i) {
    auto sock = std::make_shared<tcp::socket>(ioc_);
    client_sockets.push_back(sock);
    client_threads.emplace_back([this, sock] {
      try {
        tcp::endpoint ep(net::ip::make_address("127.0.0.1"), test_port_);
        boost::system::error_code ec;
        for (int k = 0; k < 20; ++k) {
          sock->connect(ep, ec);
          if (!ec) break;
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      } catch (...) {
      }
    });
  }

  for (auto& t : client_threads) {
    if (t.joinable()) t.join();
  }

  // Wait for all clients to connect on the server side
  int attempts = 0;
  while (server_->client_count() < client_count && attempts < 50) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    attempts++;
  }

  EXPECT_EQ(server_->client_count(), client_count);

  std::cout << "Benchmarking connected_clients() with " << client_count << " clients..." << std::endl;

  const int iterations = 100000;
  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    auto clients = server_->connected_clients();
    // Do something to prevent optimization
    if (clients.size() != client_count) {
      std::cerr << "Unexpected client count!" << std::endl;
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

  std::cout << "Iterations: " << iterations << std::endl;
  std::cout << "Time elapsed: " << duration_ms << " ms" << std::endl;
  std::cout << "Ops/sec: " << (static_cast<double>(iterations) * 1000.0 / static_cast<double>(duration_ms))
            << std::endl;
}
