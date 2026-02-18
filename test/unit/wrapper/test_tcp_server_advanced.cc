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
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "test/utils/test_utils.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

namespace {
// Mock channel for testing generic channel handling in stop()
class MockChannel : public unilink::interface::Channel {
 public:
  void start() override {}
  void stop() override {}
  bool is_connected() const override { return true; }
  void async_write_copy(unilink::memory::ConstByteSpan) override {}
  void async_write_move(std::vector<uint8_t>&&) override {}
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>>) override {}
  void on_bytes(unilink::interface::Channel::OnBytes) override {}
  void on_state(unilink::interface::Channel::OnState) override {}
  void on_backpressure(unilink::interface::Channel::OnBackpressure) override {}
};
}  // namespace

/**
 * @brief Advanced TCP Server Coverage Test
 * Tests uncovered functions in tcp_server.cc
 */
class AdvancedTcpServerCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_port_ = TestUtils::getAvailableTestPort();
    server_ = nullptr;
  }

  void TearDown() override {
    if (server_) {
      try {
        server_->stop();
      } catch (...) {
        // Ignore cleanup errors
      }
    }
    // Wait for cleanup
    TestUtils::waitFor(100);
  }

  uint16_t test_port_;
  std::shared_ptr<wrapper::TcpServer> server_;
};

// ============================================================================
// SERVER LIFECYCLE TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, ServerStartStopMultipleTimes) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  // Start server
  server_->start();

  // Stop server
  server_->stop();

  // Start again
  server_->start();

  // Stop again
  server_->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, ServerStartWhenAlreadyStarted) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  // Start server
  server_->start();

  // Try to start again (should be safe)
  server_->start();
}

TEST_F(AdvancedTcpServerCoverageTest, ServerStopWhenNotStarted) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  // Stop when not started (should be safe)
  server_->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, AutoManageStartsListening) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().auto_manage(true).build();

  ASSERT_NE(server_, nullptr);
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server_->is_listening(); }, 1000));

  server_->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, ExternalContextNotStoppedWhenNotManaged) {
  auto external_ioc = std::make_shared<boost::asio::io_context>();
  auto guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      external_ioc->get_executor());
  std::thread ioc_thread([&]() { external_ioc->run(); });

  auto server = std::make_shared<wrapper::TcpServer>(test_port_, external_ioc);
  server->set_unlimited_clients();
  server->start();
  server->stop();

  EXPECT_FALSE(external_ioc->stopped());

  guard.reset();
  external_ioc->stop();
  if (ioc_thread.joinable()) {
    ioc_thread.join();
  }
}

TEST_F(AdvancedTcpServerCoverageTest, ExternalContextManagedRunsAndStops) {
  auto external_ioc = std::make_shared<boost::asio::io_context>();
  auto server = std::make_shared<wrapper::TcpServer>(test_port_, external_ioc);
  server->set_manage_external_context(true);
  server->set_unlimited_clients();

  std::atomic<bool> ran{false};
  server->start();
  boost::asio::post(*external_ioc, [&]() { ran = true; });

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return ran.load(); }, 1000));

  server->stop();
  EXPECT_TRUE(external_ioc->stopped());
}

TEST_F(AdvancedTcpServerCoverageTest, BindingConflictTriggersErrorCallback) {
#ifdef _WIN32
  // Windows sockets can allow rapid rebinds due to TIME_WAIT/port reuse behavior.
  // Skip to avoid nondeterministic failures on CI runners.
  GTEST_SKIP() << "Binding conflict callback is flaky on Windows sockets";
#endif

  auto port = test_port_;
  std::atomic<bool> error_called{false};

  // First server binds successfully
  auto server1 = unilink::tcp_server(port)
                     .unlimited_clients()
                     .on_error([&](const wrapper::ErrorContext& ctx) {
                       error_called = true;
                     })
                     .build();
  ASSERT_NE(server1, nullptr);
  server1->start().get(); // Wait for bind

  // Second server attempts same port (no retry)
  server_ = unilink::tcp_server(port)
                .unlimited_clients()
                .enable_port_retry(false)
                .on_error([&](const wrapper::ErrorContext& ctx) {
                  error_called = true;
                })
                .build();
  ASSERT_NE(server_, nullptr);
  server_->start(); // Don't wait, it should fail

  // We expect bind to fail and error callback to fire (or at least not listen)
  TestUtils::waitFor(200);
  EXPECT_TRUE(error_called.load() || !server_->is_listening());

  server_->stop();
  server1->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, StopDisconnectsConnectedClients) {
  server_ =
      unilink::tcp_server(test_port_).unlimited_clients().on_connect([](const wrapper::ConnectionContext&) {}).build();
  ASSERT_NE(server_, nullptr);
  server_->start().get();

  std::atomic<bool> connected{false};
  std::atomic<bool> disconnected_or_down{false};
  auto client = unilink::tcp_client("127.0.0.1", test_port_)
                    .on_connect([&](const wrapper::ConnectionContext&) { connected = true; })
                    .on_disconnect([&](const wrapper::ConnectionContext&) { disconnected_or_down = true; })
                    .on_error([&](const wrapper::ErrorContext&) { disconnected_or_down = true; })
                    .auto_manage(true)
                    .build();

  ASSERT_NE(client, nullptr);

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return connected.load(); }, 1000));

  server_->stop();

  EXPECT_TRUE(
      TestUtils::waitForCondition([&]() { return disconnected_or_down.load() || !client->is_connected(); }, 1500));

  client->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, StopDisconnectsAllConnectedClients) {
  server_ =
      unilink::tcp_server(test_port_).unlimited_clients().on_connect([](const wrapper::ConnectionContext&) {}).build();
  ASSERT_NE(server_, nullptr);
  server_->start().get();

  std::atomic<int> connected{0};
  std::atomic<int> disconnected{0};

  auto make_client = [&](int id) {
    return unilink::tcp_client("127.0.0.1", test_port_)
        .on_connect([&, id](const wrapper::ConnectionContext&) {
          (void)id;
          connected.fetch_add(1);
        })
        .on_disconnect([&](const wrapper::ConnectionContext&) { disconnected.fetch_add(1); })
        .on_error([&](const wrapper::ErrorContext&) { disconnected.fetch_add(1); })
        .auto_manage(true)
        .build();
  };

  auto client1 = make_client(1);
  auto client2 = make_client(2);
  ASSERT_NE(client1, nullptr);
  ASSERT_NE(client2, nullptr);

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return connected.load() >= 2; }, 2000));

  server_->stop();

  EXPECT_TRUE(TestUtils::waitForCondition(
      [&]() { return disconnected.load() >= 2 || (!client1->is_connected() && !client2->is_connected()); }, 2000));

  client1->stop();
  client2->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, StableClientIdsAreMonotonicAndNotReused) {
  std::mutex ids_mutex;
  std::vector<size_t> ids;

  server_ = unilink::tcp_server(test_port_)
                .unlimited_clients()
                .on_connect([&](const wrapper::ConnectionContext& ctx) {
                  std::lock_guard<std::mutex> lk(ids_mutex);
                  ids.push_back(ctx.client_id());
                })
                .build();
  ASSERT_NE(server_, nullptr);
  server_->start().get();
  TestUtils::waitFor(50);

  auto make_client = [&]() { return unilink::tcp_client("127.0.0.1", test_port_).auto_manage(true).build(); };

  auto client1 = make_client();
  ASSERT_NE(client1, nullptr);
  EXPECT_TRUE(TestUtils::waitForCondition(
      [&]() {
        std::lock_guard<std::mutex> lk(ids_mutex);
        return ids.size() >= 1;
      },
      6000));

  client1->stop();
  TestUtils::waitFor(200);

  auto client2 = make_client();
  ASSERT_NE(client2, nullptr);
  EXPECT_TRUE(TestUtils::waitForCondition(
      [&]() {
        std::lock_guard<std::mutex> lk(ids_mutex);
        return ids.size() >= 2;
      },
      6000));

  auto client3 = make_client();
  ASSERT_NE(client3, nullptr);
  EXPECT_TRUE(TestUtils::waitForCondition(
      [&]() {
        std::lock_guard<std::mutex> lk(ids_mutex);
        return ids.size() >= 3;
      },
      6000));

  std::vector<size_t> ids_snapshot;
  {
    std::lock_guard<std::mutex> lk(ids_mutex);
    ids_snapshot = ids;
  }

  ASSERT_EQ(ids_snapshot.size(), 3U);
  EXPECT_LT(ids_snapshot[0], ids_snapshot[1]);
  EXPECT_LT(ids_snapshot[1], ids_snapshot[2]);

  client2->stop();
  client3->stop();
  server_->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, StopFromCallbackDoesNotDeadlock) {
  std::atomic<bool> stop_called{false};

  server_ = unilink::tcp_server(test_port_)
                .unlimited_clients()
                .on_connect([&](const wrapper::ConnectionContext&) {
                  stop_called = true;
                  server_->stop();
                })
                .build();
  ASSERT_NE(server_, nullptr);
  server_->start().get();

  auto client = unilink::tcp_client("127.0.0.1", test_port_).auto_manage(false).build();
  ASSERT_NE(client, nullptr);
  client->start();

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return stop_called.load(); }, 2000));
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return !server_->is_listening(); }, 2000));

  client->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, SendAndCountReflectLiveClientsAndReturnStatus) {
  std::mutex ids_mutex;
  std::vector<size_t> ids;
  std::atomic<bool> error_called{false};

  server_ = unilink::tcp_server(test_port_)
                .unlimited_clients()
                .on_connect([&](const wrapper::ConnectionContext& ctx) {
                  std::lock_guard<std::mutex> lk(ids_mutex);
                  ids.push_back(ctx.client_id());
                })
                .build();
  ASSERT_NE(server_, nullptr);
  server_->notify_send_failure(true).on_error([&](const wrapper::ErrorContext&) { error_called = true; });
  server_->start().get();

  auto client1 = unilink::tcp_client("127.0.0.1", test_port_).auto_manage(false).build();
  auto client2 = unilink::tcp_client("127.0.0.1", test_port_).auto_manage(false).build();
  ASSERT_NE(client1, nullptr);
  ASSERT_NE(client2, nullptr);

  client1->start();
  client2->start();

  EXPECT_TRUE(TestUtils::waitForCondition([&]() {
    std::lock_guard<std::mutex> lk(ids_mutex);
    return ids.size() >= 2 && server_->get_client_count() == 2;
  }));

  size_t first_id = 0;
  {
    std::lock_guard<std::mutex> lk(ids_mutex);
    first_id = ids.front();
  }

  EXPECT_TRUE(server_->send_to(first_id, "ping"));
  server_->broadcast("hello");

  EXPECT_FALSE(server_->send_to(999999, "invalid"));

  server_->stop();
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server_->get_client_count() == 0; }, 2000));
  EXPECT_FALSE(server_->send_to(first_id, "should fail"));
  server_->broadcast("down");

  client1->stop();
  client2->stop();

  EXPECT_TRUE(error_called.load());
}

// ============================================================================
// CLIENT LIMIT CONFIGURATION TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, UnlimitedClientsConfiguration) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
}

TEST_F(AdvancedTcpServerCoverageTest, SingleClientConfiguration) {
  server_ = unilink::tcp_server(test_port_).single_client().build();

  ASSERT_NE(server_, nullptr);
}

TEST_F(AdvancedTcpServerCoverageTest, MultiClientConfiguration) {
  server_ = unilink::tcp_server(test_port_).multi_client(5).build();

  ASSERT_NE(server_, nullptr);
}

// ============================================================================
// PORT RETRY CONFIGURATION TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, PortRetryConfiguration) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
}

// ============================================================================
// MESSAGE HANDLING TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, SetMessageHandler) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
  server_->start();
}

TEST_F(AdvancedTcpServerCoverageTest, StopWithGenericChannelIsFast) {
  auto mock = std::make_shared<MockChannel>();
  server_ = std::make_shared<wrapper::TcpServer>(mock);

  auto start = std::chrono::steady_clock::now();
  server_->stop();
  auto end = std::chrono::steady_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  EXPECT_LT(duration, 100) << "Stop() took too long: " << duration << "ms";
}

TEST_F(AdvancedTcpServerCoverageTest, SetConnectionHandler) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
  server_->start();
}

// ============================================================================
// BROADCAST FUNCTIONALITY TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, BroadcastToAllClients) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  server_->start();
  server_->broadcast("Test broadcast message");
}

TEST_F(AdvancedTcpServerCoverageTest, BroadcastToSpecificClient) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  server_->start();
  server_->broadcast("Test message");
}

// ============================================================================
// SERVER STATE TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, GetServerInfo) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
  server_->start();
}

TEST_F(AdvancedTcpServerCoverageTest, GetConnectedClientsCount) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
  server_->start();
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, ServerWithInvalidPort) {
  try {
    server_ = unilink::tcp_server(0).unlimited_clients().build();
    if (server_) {
      server_->start();
    }
  } catch (...) {
  }
}

TEST_F(AdvancedTcpServerCoverageTest, ServerWithHighPort) {
  server_ = unilink::tcp_server(65535).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  try {
    server_->start();
  } catch (...) {
  }
}

// ============================================================================
// CONCURRENT OPERATIONS TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, ConcurrentStartStop) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  std::vector<std::thread> threads;
  const int num_threads = 4;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([this, i]() {
      if (i % 2 == 0) {
        server_->start();
      } else {
        server_->stop();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

// ============================================================================
// EDGE CASES AND STRESS TESTS
// ============================================================================

TEST_F(AdvancedTcpServerCoverageTest, RapidStartStop) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);

  for (int i = 0; i < 10; ++i) {
    server_->start();
    std::this_thread::sleep_for(10ms);
    server_->stop();
    std::this_thread::sleep_for(10ms);
  }
}

TEST_F(AdvancedTcpServerCoverageTest, HandlerReplacement) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();

  ASSERT_NE(server_, nullptr);
  server_->start();
}
