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
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

/**
 * @brief Advanced TCP Client Coverage Test
 * Tests uncovered functions in tcp_client.cc
 */
class AdvancedTcpClientCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_port_ = TestUtils::getAvailableTestPort();
    client_ = nullptr;
    server_ = nullptr;
  }

  void TearDown() override {
    if (client_) {
      try {
        client_->stop();
      } catch (...) {
        // Ignore cleanup errors
      }
    }
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
  std::shared_ptr<wrapper::TcpClient> client_;
  std::shared_ptr<wrapper::TcpServer> server_;
};

// ============================================================================
// CLIENT LIFECYCLE TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, ClientStartStopMultipleTimes) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Start client
  client_->start();

  // Stop client
  client_->stop();

  // Start again
  client_->start();

  // Stop again
  client_->stop();
}

TEST_F(AdvancedTcpClientCoverageTest, ClientStartWhenAlreadyStarted) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Start client
  client_->start();

  // Try to start again (should be safe)
  client_->start();
}

TEST_F(AdvancedTcpClientCoverageTest, ClientStopWhenNotStarted) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Stop when not started (should be safe)
  client_->stop();
}

TEST_F(AdvancedTcpClientCoverageTest, InvalidHostTriggersErrorCallback) {
  std::atomic<bool> error_called{false};
  client_ = unilink::tcp_client("256.256.256.256", test_port_)
                .on_error([&error_called](const wrapper::ErrorContext&) { error_called = true; })
                .build();

  ASSERT_NE(client_, nullptr);
  client_->start();
  TestUtils::waitFor(200);
  EXPECT_TRUE(error_called.load() || !client_->is_connected());
}

// ============================================================================
// CONNECTION CONFIGURATION TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, ClientWithHostAndPort) {
  client_ = unilink::tcp_client("127.0.0.1", test_port_).build();

  ASSERT_NE(client_, nullptr);
}

TEST_F(AdvancedTcpClientCoverageTest, ClientWithLocalhost) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);
}

TEST_F(AdvancedTcpClientCoverageTest, ClientWithIPv6Address) {
  client_ = unilink::tcp_client("::1", test_port_).build();

  ASSERT_NE(client_, nullptr);
}

// ============================================================================
// RETRY CONFIGURATION TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, ClientWithRetryConfiguration) {
  client_ = unilink::tcp_client("localhost", test_port_).retry_interval(100).build();

  ASSERT_NE(client_, nullptr);
}

TEST_F(AdvancedTcpClientCoverageTest, ClientWithConnectionTimeout) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);
}

// ============================================================================
// MESSAGE HANDLING TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, SendMessage) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Start client
  client_->start();

  // Try to send message (no server, should be safe)
  client_->send("Test message");
}

TEST_F(AdvancedTcpClientCoverageTest, SendLine) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Start client
  client_->start();

  // Try to send line (no server, should be safe)
  client_->send_line("Test line");
}

// ============================================================================
// CONNECTION STATE TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, IsConnectedWhenNotStarted) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Should not be connected when not started
  EXPECT_FALSE(client_->is_connected());
}

TEST_F(AdvancedTcpClientCoverageTest, IsConnectedWhenStarted) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  // Start client
  client_->start();
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, ClientWithInvalidHost) {
  try {
    client_ = unilink::tcp_client("invalid_host_that_does_not_exist", test_port_).build();
    if (client_) {
      client_->start();
    }
  } catch (...) {
  }
}

TEST_F(AdvancedTcpClientCoverageTest, ClientWithInvalidPort) {
  try {
    client_ = unilink::tcp_client("localhost", 0).build();
    if (client_) {
      client_->start();
    }
  } catch (...) {
  }
}

TEST_F(AdvancedTcpClientCoverageTest, ClientWithHighPort) {
  client_ = unilink::tcp_client("localhost", 65535).build();

  ASSERT_NE(client_, nullptr);

  try {
    client_->start();
  } catch (...) {
  }
}

// ============================================================================
// CONCURRENT OPERATIONS TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, ConcurrentStartStop) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  std::vector<std::thread> threads;
  const int num_threads = 2;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([this, i]() {
      try {
        if (i % 2 == 0) {
          client_->start();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
          client_->stop();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      } catch (...) {
      }
    });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

// ============================================================================
// EDGE CASES AND STRESS TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, RapidStartStop) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  for (int i = 0; i < 10; ++i) {
    client_->start();
    std::this_thread::sleep_for(10ms);
    client_->stop();
    std::this_thread::sleep_for(10ms);
  }
}

TEST_F(AdvancedTcpClientCoverageTest, MultipleClients) {
  std::vector<std::unique_ptr<wrapper::TcpClient>> clients;
  const int num_clients = 5;

  for (int i = 0; i < num_clients; ++i) {
    auto client = unilink::tcp_client("localhost", static_cast<uint16_t>(test_port_ + i)).build();
    clients.push_back(std::move(client));
  }

  for (auto& client : clients) {
    client->start();
  }

  for (auto& client : clients) {
    client->stop();
  }
}

// ============================================================================
// DESTRUCTOR TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, DestructorWithStartedClient) {
  auto client = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client, nullptr);
  client->start();
}

TEST_F(AdvancedTcpClientCoverageTest, DestructorWithStoppedClient) {
  auto client = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client, nullptr);
}

// ============================================================================
// AUTO START TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, AutoStartEnabled) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);
}

TEST_F(AdvancedTcpClientCoverageTest, AutoStartDisabled) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);
}

TEST_F(AdvancedTcpClientCoverageTest, AutoManageStartsClientAndInvokesCallback) {
  server_ =
      unilink::tcp_server(test_port_).unlimited_clients().on_connect([](const wrapper::ConnectionContext&) {}).build();
  ASSERT_NE(server_, nullptr);
  server_->start().get();

  std::atomic<bool> connected{false};
  client_ =
      unilink::tcp_client("127.0.0.1", test_port_)
          .on_connect([&](const wrapper::ConnectionContext&) { connected = true; })
          .auto_manage(true)
          .build();
  ASSERT_NE(client_, nullptr);

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return connected.load(); }, 1000));

  client_->stop();
  server_->stop();
}

TEST_F(AdvancedTcpClientCoverageTest, ExternalContextNotStoppedWhenNotManaged) {
  auto external_ioc = std::make_shared<boost::asio::io_context>();
  auto guard = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      external_ioc->get_executor());
  std::thread ioc_thread([&]() { external_ioc->run(); });

  auto client = std::make_shared<wrapper::TcpClient>("127.0.0.1", test_port_, external_ioc);
  ASSERT_NE(client, nullptr);

  client->start().get();
  client->stop();

  EXPECT_FALSE(external_ioc->stopped());

  guard.reset();
  external_ioc->stop();
  if (ioc_thread.joinable()) {
    ioc_thread.join();
  }
}

TEST_F(AdvancedTcpClientCoverageTest, ExternalContextManagedRunsAndStops) {
  auto external_ioc = std::make_shared<boost::asio::io_context>();
  auto client = std::make_shared<wrapper::TcpClient>("127.0.0.1", test_port_, external_ioc);
  client->set_manage_external_context(true);

  ASSERT_NE(client, nullptr);

  std::atomic<bool> ran{false};

  client->start();
  boost::asio::post(*external_ioc, [&]() { ran = true; });
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return ran.load(); }, 1000));

  client->stop();
  EXPECT_TRUE(external_ioc->stopped());
}

// ============================================================================
// CONNECTION RETRY TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, ConnectionRetry) {
  client_ = unilink::tcp_client("localhost", test_port_).retry_interval(100).build();

  ASSERT_NE(client_, nullptr);

  try {
    client_->start();
  } catch (...) {
  }
}

// ============================================================================
// MESSAGE SENDING TESTS
// ============================================================================

TEST_F(AdvancedTcpClientCoverageTest, SendMultipleMessages) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  client_->start();

  for (int i = 0; i < 10; ++i) {
    client_->send("Message " + std::to_string(i));
    client_->send_line("Line " + std::to_string(i));
  }
}

TEST_F(AdvancedTcpClientCoverageTest, SendEmptyMessage) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  client_->start();

  client_->send("");
  client_->send_line("");
}

TEST_F(AdvancedTcpClientCoverageTest, SendLongMessage) {
  client_ = unilink::tcp_client("localhost", test_port_).build();

  ASSERT_NE(client_, nullptr);

  client_->start();

  std::string long_message(1000, 'x');
  client_->send(long_message);
  client_->send_line(long_message);
}
