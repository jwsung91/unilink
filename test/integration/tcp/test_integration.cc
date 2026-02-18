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
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "test_utils.hpp"
#include "unilink/builder/auto_initializer.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

// Helper fixture for Integration Tests
class TcpIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override { test_port_ = TestUtils::getAvailableTestPort(); }
  uint16_t test_port_;
};

// ============================================================================
// BUILDER INTEGRATION TESTS
// ============================================================================

TEST_F(TcpIntegrationTest, BuilderPatternIntegration) {
  auto server = unilink::tcp_server(test_port_).unlimited_clients().build();
  EXPECT_NE(server, nullptr);

  auto client = unilink::tcp_client("127.0.0.1", test_port_).build();
  EXPECT_NE(client, nullptr);
}

TEST_F(TcpIntegrationTest, AutoInitialization) {
  builder::AutoInitializer::ensure_io_context_running();
  TestUtils::waitFor(100);
  EXPECT_TRUE(builder::AutoInitializer::is_io_context_running());
}

TEST_F(TcpIntegrationTest, MethodChaining) {
  auto client =
      unilink::tcp_client("127.0.0.1", test_port_)
          .auto_manage(false)
          .on_connect([](const wrapper::ConnectionContext&) { std::cout << "Connected!" << std::endl; })
          .on_disconnect([](const wrapper::ConnectionContext&) { std::cout << "Disconnected!" << std::endl; })
          .on_data([](const wrapper::MessageContext& ctx) { std::cout << "Data: " << ctx.data() << std::endl; })
          .on_error([](const wrapper::ErrorContext& ctx) { std::cout << "Error: " << ctx.message() << std::endl; })
          .build();

  EXPECT_NE(client, nullptr);
}

TEST_F(TcpIntegrationTest, IndependentContext) {
  auto client = unilink::tcp_client("127.0.0.1", test_port_).use_independent_context(true).build();
  EXPECT_NE(client, nullptr);

  auto server = unilink::tcp_server(test_port_).unlimited_clients().use_independent_context(false).build();
  EXPECT_NE(server, nullptr);
}

// ============================================================================
// COMMUNICATION TESTS
// ============================================================================

TEST_F(TcpIntegrationTest, BasicCommunication) {
  uint16_t comm_port = TestUtils::getAvailableTestPort();

  std::atomic<bool> server_connected{false};
  std::atomic<bool> client_connected{false};
  std::atomic<bool> data_received{false};
  std::string received_data;

  auto server = unilink::tcp_server(comm_port)
                    .unlimited_clients()
                    .on_connect([&server_connected](const wrapper::ConnectionContext&) { server_connected = true; })
                    .on_data([&data_received, &received_data](const wrapper::MessageContext& ctx) {
                      received_data = std::string(ctx.data());
                      data_received = true;
                    })
                    .build();

  ASSERT_NE(server, nullptr);
  EXPECT_TRUE(server->start().get());

  auto client = unilink::tcp_client("127.0.0.1", comm_port)
                    .on_connect([&client_connected](const wrapper::ConnectionContext&) { client_connected = true; })
                    .build();

  ASSERT_NE(client, nullptr);
  client->start();

  EXPECT_TRUE(TestUtils::waitForCondition([&client_connected]() { return client_connected.load(); }, 5000));

  if (client->is_connected()) {
    TestUtils::waitFor(100);
    client->send("test message");

    EXPECT_TRUE(TestUtils::waitForCondition([&data_received]() { return data_received.load(); }, 5000));
    if (data_received.load()) {
      EXPECT_EQ(received_data, "test message");
    }
  }
}

TEST_F(TcpIntegrationTest, ErrorHandling) {
  // Test invalid port (now mostly caught by InputValidator if used, but let's test runtime)
  // unilink::tcp_server(0) might throw or return nullptr depending on version
  try {
    auto server = unilink::tcp_server(0).build();
    if (server) server->start();
  } catch (...) {
  }

  std::atomic<bool> error_occurred{false};
  auto client = unilink::tcp_client("127.0.0.1", 1)
                    .on_error([&error_occurred](const wrapper::ErrorContext&) { error_occurred = true; })
                    .build();

  EXPECT_NE(client, nullptr);
  client->start();
  TestUtils::waitFor(200);
  EXPECT_TRUE(error_occurred.load() || !client->is_connected());
}

// ============================================================================
// ARCHITECTURE TESTS
// ============================================================================

TEST_F(TcpIntegrationTest, ResourceSharing) {
  std::vector<std::unique_ptr<wrapper::TcpClient>> clients;
  for (int i = 0; i < 3; ++i) {
    auto client = unilink::tcp_client("127.0.0.1", test_port_).build();
    EXPECT_NE(client, nullptr);
    clients.push_back(std::move(client));
  }
  EXPECT_EQ(clients.size(), 3);
}

TEST_F(TcpIntegrationTest, MultipleClientConnections) {
  uint16_t comm_port = TestUtils::getAvailableTestPort();
  std::atomic<int> connection_count{0};

  auto server = unilink::tcp_server(comm_port)
                    .unlimited_clients()
                    .on_connect([&connection_count](const wrapper::ConnectionContext&) { connection_count++; })
                    .build();

  ASSERT_NE(server, nullptr);
  server->start();

  std::vector<std::unique_ptr<wrapper::TcpClient>> clients;
  for (int i = 0; i < 3; ++i) {
    auto client = unilink::tcp_client("127.0.0.1", comm_port).auto_manage(true).build();
    clients.push_back(std::move(client));
    TestUtils::waitFor(100);
  }

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return connection_count.load() >= 3; }, 5000));
}

TEST_F(TcpIntegrationTest, ComprehensiveBuilderMethodChaining) {
  auto client = unilink::tcp_client("127.0.0.1", test_port_)
                    .auto_manage(false)
                    .use_independent_context(true)
                    .on_connect([](const wrapper::ConnectionContext&) {})
                    .on_data([](const wrapper::MessageContext&) {})
                    .build();

  EXPECT_NE(client, nullptr);

  auto server = unilink::tcp_server(test_port_)
                    .unlimited_clients()
                    .auto_manage(false)
                    .on_connect([](const wrapper::ConnectionContext&) {})
                    .on_data([](const wrapper::MessageContext&) {})
                    .build();

  EXPECT_NE(server, nullptr);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
