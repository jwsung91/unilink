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
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include "test_utils.hpp"
#include "unilink/memory/safe_span.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;

namespace net = boost::asio;
using tcp = net::ip::tcp;

TEST_F(IntegrationTest, TcpClientStopFromCallbackDoesNotDeadlock) {
  net::io_context server_ioc;
  auto guard = net::make_work_guard(server_ioc);
  tcp::acceptor acceptor(server_ioc, tcp::endpoint(tcp::v4(), test_port_));
  auto server_socket = std::make_shared<tcp::socket>(server_ioc);

  std::atomic<int> state_notifications{0};
  std::atomic<bool> stop_from_state{false};
  std::atomic<bool> stop_from_bytes{false};

  // No terminal_notifications or terminal_reached logic as per Stop Semantics contract

  acceptor.async_accept(*server_socket, [&](const boost::system::error_code& ec) {
    if (ec) return;
    net::post(server_ioc, [server_socket]() {
      boost::system::error_code send_ec;
      net::write(*server_socket, net::buffer("ping", 4), send_ec);
    });
  });

  std::thread server_thread([&]() {
    try {
      server_ioc.run();
    } catch (...) {
    }
  });

  TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = test_port_;
  cfg.connection_timeout_ms = 500;
  cfg.retry_interval_ms = 50;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg);
  std::weak_ptr<TcpClient> weak_client = client;

  client->on_state([weak_client, &state_notifications, &stop_from_state](LinkState state) {
    state_notifications.fetch_add(1);
    if (state == LinkState::Connected) {
      stop_from_state.store(true);
      if (auto c = weak_client.lock()) {
        c->stop();
      }
    }
    // No terminal state check here, as per Contract
  });
  client->on_bytes([weak_client, &stop_from_bytes](memory::ConstByteSpan) {
    stop_from_bytes.store(true);
    if (auto c = weak_client.lock()) {
      c->stop();
    }
  });

  client->start();

  // Wait long enough for client to attempt connection and then get stopped from callback
  TestUtils::waitFor(1000);  // Give enough time for the Connected state to be reached and stop() called

  // Explicitly stop client at the end of the test to ensure cleanup
  client->stop();

  // Verify that the client is no longer connected (or attempts to connect)
  EXPECT_FALSE(client->is_connected());
  // Also, check that stop was indeed called from one of the callbacks
  EXPECT_TRUE(stop_from_state.load() || stop_from_bytes.load());

  guard.reset();
  server_ioc.stop();
  if (server_thread.joinable()) server_thread.join();
}
