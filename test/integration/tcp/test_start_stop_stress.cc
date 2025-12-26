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
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;

namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST_F(IntegrationTest, TcpClientStartStopStress) {
  net::io_context server_ioc;
  auto guard = net::make_work_guard(server_ioc);
  tcp::acceptor acceptor(server_ioc, tcp::endpoint(tcp::v4(), test_port_));

  std::mutex server_mutex;
  std::vector<std::shared_ptr<tcp::socket>> server_sockets;

  std::function<void()> accept_once;
  accept_once = [&]() {
    auto socket = std::make_shared<tcp::socket>(server_ioc);
    acceptor.async_accept(*socket, [&, socket](const boost::system::error_code& ec) {
      if (ec) {
        return;
      }
      std::lock_guard<std::mutex> lock(server_mutex);
      server_sockets.push_back(socket);
      accept_once();
    });
  };
  accept_once();

  std::thread server_thread([&]() {
    try {
      server_ioc.run();
    } catch (...) {
    }
  });

  TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = test_port_;
  cfg.connection_timeout_ms = 200;
  cfg.retry_interval_ms = 50;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg);
  std::atomic<int> terminal_notifications{0};

  client->on_state([&](LinkState state) {
    if (state == LinkState::Closed || state == LinkState::Error) {
      terminal_notifications.fetch_add(1);
    }
  });

  const int iterations = 200;
  for (int i = 0; i < iterations; ++i) {
    client->start();
    EXPECT_TRUE(
        TestUtils::waitForCondition([&] { return client->is_connected() || terminal_notifications.load() > i; }, 2000));
    client->stop();
    EXPECT_TRUE(TestUtils::waitForCondition([&] { return terminal_notifications.load() >= i + 1; }, 2000));
  }

  EXPECT_EQ(terminal_notifications.load(), iterations);

  client->stop();

  {
    std::lock_guard<std::mutex> lock(server_mutex);
    for (auto& socket : server_sockets) {
      if (socket && socket->is_open()) {
        boost::system::error_code ec;
        socket->close(ec);
      }
    }
    server_sockets.clear();
  }

  guard.reset();
  server_ioc.stop();
  if (server_thread.joinable()) server_thread.join();
}
