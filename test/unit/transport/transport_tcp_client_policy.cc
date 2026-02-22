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
#include <cstdint>
#include <memory>
#include <vector>

#include "test/utils/test_utils.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_client/reconnect_policy.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;
using namespace std::chrono_literals;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TransportTcpClientPolicyTest : public ::testing::Test {
 protected:
  void TearDown() override {
    if (client_) {
      client_->stop();
      client_.reset();
    }
    TestUtils::waitFor(50);
  }

  std::shared_ptr<TcpClient> client_;
};

TEST_F(TransportTcpClientPolicyTest, FixedIntervalPolicyRetriesWithDelay) {
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = TestUtils::getAvailableTestPort();
  cfg.retry_interval_ms = 1000;
  cfg.connection_timeout_ms = 50;

  client_ = TcpClient::create(cfg, ioc);

  client_->set_reconnect_policy(FixedInterval(20ms));

  std::atomic<int> connecting_count{0};
  std::vector<std::chrono::steady_clock::time_point> attempt_times;

  client_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Connecting) {
      connecting_count.fetch_add(1);
      attempt_times.push_back(std::chrono::steady_clock::now());
    }
  });

  client_->start();

  ioc.run_for(std::chrono::milliseconds(300));

  EXPECT_GE(connecting_count.load(), 3);

  if (attempt_times.size() >= 3) {
      for (size_t i = 1; i < attempt_times.size(); ++i) {
          auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(attempt_times[i] - attempt_times[i-1]).count();
          EXPECT_LT(diff, 500) << "Interval too long, policy might not be active";
      }
  }

  client_->stop();
  client_.reset();
}

TEST_F(TransportTcpClientPolicyTest, ExponentialBackoffPolicyIncreasesDelay) {
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = TestUtils::getAvailableTestPort();
  cfg.connection_timeout_ms = 20;

  client_ = TcpClient::create(cfg, ioc);

  client_->set_reconnect_policy(ExponentialBackoff(20ms, 1000ms, 2.0, false));

  std::vector<std::chrono::steady_clock::time_point> attempt_times;

  client_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Connecting) {
      attempt_times.push_back(std::chrono::steady_clock::now());
    }
  });

  client_->start();

  ioc.run_for(std::chrono::milliseconds(500));

  client_->stop();

  EXPECT_GE(attempt_times.size(), 3);

  if (attempt_times.size() >= 3) {
      auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>(attempt_times[1] - attempt_times[0]).count();
      auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(attempt_times[2] - attempt_times[1]).count();

      EXPECT_GT(d2, d1);
  }
}

TEST_F(TransportTcpClientPolicyTest, PolicyCanStopRetries) {
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = TestUtils::getAvailableTestPort();
  cfg.connection_timeout_ms = 20;
  cfg.max_retries = -1; // Infinite in config

  client_ = TcpClient::create(cfg, ioc);

  client_->set_reconnect_policy([](const diagnostics::ErrorInfo&, uint32_t attempt) -> ReconnectDecision {
      if (attempt >= 2) return {false, 0ms};
      return {true, 10ms};
  });

  std::atomic<int> connecting_count{0};
  std::atomic<bool> error_state{false};

  client_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Connecting) connecting_count.fetch_add(1);
    if (state == base::LinkState::Error) error_state = true;
  });

  client_->start();

  ioc.run_for(std::chrono::milliseconds(500));

  EXPECT_EQ(connecting_count.load(), 3);
  EXPECT_TRUE(error_state.load());

  client_->stop();
  client_.reset();
}

TEST_F(TransportTcpClientPolicyTest, ResetAttemptCountOnSuccess) {
    boost::asio::io_context ioc;
    // Start acceptor
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
    auto port = acceptor.local_endpoint().port();

    config::TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = port;
    cfg.connection_timeout_ms = 100;

    client_ = TcpClient::create(cfg, ioc);

    // Policy: only allow attempt 0 (1 retry)
    client_->set_reconnect_policy([](const diagnostics::ErrorInfo&, uint32_t attempt) -> ReconnectDecision {
        if (attempt >= 1) return {false, 0ms};
        return {true, 10ms};
    });

    std::atomic<int> connecting_count{0};
    client_->on_state([&](base::LinkState state) {
        if (state == base::LinkState::Connecting) connecting_count.fetch_add(1);
    });

    std::shared_ptr<tcp::socket> peer_socket;
    acceptor.async_accept([&](auto ec, tcp::socket socket) {
        if (!ec) {
            peer_socket = std::make_shared<tcp::socket>(std::move(socket));
        }
    });

    client_->start();

    // Connect successfully
    ioc.run_for(100ms);
    EXPECT_TRUE(client_->is_connected());

    // Close server
    if (peer_socket) peer_socket->close();
    acceptor.close(); // Ensure it cannot reconnect

    // Allow retry logic to run
    ioc.run_for(500ms);

    // Initial(1) + Disconnect->Connecting(1) + ScheduleRetry->Connecting(1) = 3.
    // Note: handle_close transitions to Connecting, and schedule_retry also transitions to Connecting.
    // This results in double notification for retries triggered by disconnect.
    EXPECT_EQ(connecting_count.load(), 3);

    client_->stop();
    client_.reset();
}
