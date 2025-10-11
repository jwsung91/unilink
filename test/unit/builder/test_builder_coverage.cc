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
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "unilink/common/exceptions.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

/**
 * @brief Builder Coverage Test - 테스트되지 않은 Builder 메서드들 커버리지 확보
 */
class BuilderCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset state
    multi_connect_count_ = 0;
    multi_disconnect_count_ = 0;
    multi_data_received_.clear();
  }

  void TearDown() override {
    if (server_) {
      server_->stop();
      server_.reset();
    }
    if (client_) {
      client_->stop();
      client_.reset();
    }
    std::this_thread::sleep_for(100ms);
  }

  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{10000};
    return port_counter.fetch_add(1);
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
  std::atomic<int> multi_connect_count_{0};
  std::atomic<int> multi_disconnect_count_{0};
  std::vector<std::string> multi_data_received_;
};

// ============================================================================
// TCP SERVER BUILDER COVERAGE TESTS
// ============================================================================

/**
 * @brief Test multi-client callbacks
 */
TEST_F(BuilderCoverageTest, TcpServerMultiClientCallbacks) {
  uint16_t port = getTestPort();

  // Create server with multi-client callbacks
  server_ = unilink::tcp_server(port)
                .multi_client(3)

                .on_multi_connect([this](size_t client_id, const std::string& ip) {
                  multi_connect_count_++;
                  EXPECT_GT(client_id, 0u);
                })
                .on_multi_data([this](size_t client_id, const std::string& data) {
                  multi_data_received_.push_back(data);
                  EXPECT_GT(client_id, 0u);
                })
                .on_multi_disconnect([this](size_t client_id) {
                  multi_disconnect_count_++;
                  EXPECT_GT(client_id, 0u);
                })
                .build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test port retry configuration
 */
TEST_F(BuilderCoverageTest, TcpServerPortRetry) {
  uint16_t port = getTestPort();

  // Create server with port retry enabled
  server_ = unilink::tcp_server(port)
                .unlimited_clients()

                .enable_port_retry(true, 5, 500)  // 5 retries, 500ms interval
                .build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test max_clients with various values
 */
TEST_F(BuilderCoverageTest, TcpServerMaxClients) {
  uint16_t port = getTestPort();

  // Test max_clients(2)
  auto server1 = unilink::tcp_server(port).max_clients(2).build();
  EXPECT_NE(server1, nullptr);

  // Test max_clients(5)
  auto server2 = unilink::tcp_server(port + 1).max_clients(5).build();
  EXPECT_NE(server2, nullptr);

  // Test max_clients(100)
  auto server3 = unilink::tcp_server(port + 2).max_clients(100).build();
  EXPECT_NE(server3, nullptr);

  server1->stop();
  server2->stop();
  server3->stop();
}

/**
 * @brief Test max_clients with invalid values
 */
TEST_F(BuilderCoverageTest, TcpServerMaxClientsInvalid) {
  uint16_t port = getTestPort();

  // Test max_clients(0) - should throw
  EXPECT_THROW({ unilink::tcp_server(port).max_clients(0).build(); }, std::invalid_argument);

  // Test max_clients(1) - should throw
  EXPECT_THROW({ unilink::tcp_server(port).max_clients(1).build(); }, std::invalid_argument);
}

/**
 * @brief Test single_client mode
 */
TEST_F(BuilderCoverageTest, TcpServerSingleClient) {
  uint16_t port = getTestPort();

  server_ = unilink::tcp_server(port).single_client().build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test multi_client mode with specific limit
 */
TEST_F(BuilderCoverageTest, TcpServerMultiClientLimit) {
  uint16_t port = getTestPort();

  // Valid multi_client values
  auto server1 = unilink::tcp_server(port).multi_client(2).build();
  EXPECT_NE(server1, nullptr);

  auto server2 = unilink::tcp_server(port + 1).multi_client(10).build();
  EXPECT_NE(server2, nullptr);

  server1->stop();
  server2->stop();
}

/**
 * @brief Test multi_client with invalid values
 */
TEST_F(BuilderCoverageTest, TcpServerMultiClientInvalid) {
  uint16_t port = getTestPort();

  // multi_client(0) - should throw
  EXPECT_THROW({ unilink::tcp_server(port).multi_client(0).build(); }, std::invalid_argument);

  // multi_client(1) - should throw
  EXPECT_THROW({ unilink::tcp_server(port).multi_client(1).build(); }, std::invalid_argument);
}

/**
 * @brief Test use_independent_context
 */
TEST_F(BuilderCoverageTest, TcpServerIndependentContext) {
  uint16_t port = getTestPort();

  server_ = unilink::tcp_server(port).unlimited_clients().use_independent_context(true).build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test on_data overload with client_id
 */
TEST_F(BuilderCoverageTest, TcpServerOnDataWithClientId) {
  uint16_t port = getTestPort();

  std::atomic<bool> data_received{false};

  server_ = unilink::tcp_server(port)
                .unlimited_clients()

                .on_data([&data_received](size_t client_id, const std::string& data) {
                  data_received = true;
                  EXPECT_GT(client_id, 0u);
                  EXPECT_FALSE(data.empty());
                })
                .build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test on_connect overload with client info
 */
TEST_F(BuilderCoverageTest, TcpServerOnConnectWithClientInfo) {
  uint16_t port = getTestPort();

  std::atomic<bool> connected{false};

  server_ = unilink::tcp_server(port)
                .unlimited_clients()

                .on_connect([&connected](size_t client_id, const std::string& ip) {
                  connected = true;
                  EXPECT_GT(client_id, 0u);
                  EXPECT_FALSE(ip.empty());
                })
                .build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test on_disconnect overload with client_id
 */
TEST_F(BuilderCoverageTest, TcpServerOnDisconnectWithClientId) {
  uint16_t port = getTestPort();

  std::atomic<bool> disconnected{false};

  server_ = unilink::tcp_server(port)
                .unlimited_clients()

                .on_disconnect([&disconnected](size_t client_id) {
                  disconnected = true;
                  EXPECT_GT(client_id, 0u);
                })
                .build();

  EXPECT_NE(server_, nullptr);
}

/**
 * @brief Test auto_manage configuration
 */
TEST_F(BuilderCoverageTest, TcpServerAutoManage) {
  uint16_t port = getTestPort();

  // auto_manage(true)
  auto server1 = unilink::tcp_server(port).unlimited_clients().auto_manage(true).build();
  EXPECT_NE(server1, nullptr);

  // auto_manage(false)
  auto server2 = unilink::tcp_server(port + 1).unlimited_clients().auto_manage(false).build();
  EXPECT_NE(server2, nullptr);

  server1->stop();
  server2->stop();
}

// ============================================================================
// TCP CLIENT BUILDER COVERAGE TESTS
// ============================================================================

/**
 * @brief Test TcpClient auto_manage
 */
TEST_F(BuilderCoverageTest, TcpClientAutoManage) {
  uint16_t port = getTestPort();

  // auto_manage(true)
  auto client1 = unilink::tcp_client("127.0.0.1", port).auto_manage(true).build();
  EXPECT_NE(client1, nullptr);

  // auto_manage(false)
  auto client2 = unilink::tcp_client("127.0.0.1", port + 1).auto_manage(false).build();
  EXPECT_NE(client2, nullptr);

  client1->stop();
  client2->stop();
}

/**
 * @brief Test TcpClient with callbacks
 */
TEST_F(BuilderCoverageTest, TcpClientCallbacks) {
  uint16_t port = getTestPort();

  std::atomic<bool> data_received{false};
  std::atomic<bool> connected{false};
  std::atomic<bool> disconnected{false};
  std::atomic<bool> error_occurred{false};

  client_ = unilink::tcp_client("127.0.0.1", port)

                .on_data([&data_received](const std::string& data) { data_received = true; })
                .on_connect([&connected]() { connected = true; })
                .on_disconnect([&disconnected]() { disconnected = true; })
                .on_error([&error_occurred](const std::string& error) { error_occurred = true; })
                .build();

  EXPECT_NE(client_, nullptr);
}

/**
 * @brief Test TcpClient use_independent_context
 */
TEST_F(BuilderCoverageTest, TcpClientIndependentContext) {
  uint16_t port = getTestPort();

  client_ = unilink::tcp_client("127.0.0.1", port).use_independent_context(true).build();

  EXPECT_NE(client_, nullptr);
}

// ============================================================================
// SERIAL BUILDER COVERAGE TESTS
// ============================================================================

/**
 * @brief Test Serial auto_manage
 */
TEST_F(BuilderCoverageTest, SerialAutoManage) {
  // auto_manage(true)
  auto serial1 = unilink::serial("/dev/null", 9600).auto_manage(true).build();
  EXPECT_NE(serial1, nullptr);

  // auto_manage(false)
  auto serial2 = unilink::serial("/dev/null", 9600).auto_manage(false).build();
  EXPECT_NE(serial2, nullptr);

  serial1->stop();
  serial2->stop();
}

/**
 * @brief Test Serial with callbacks
 */
TEST_F(BuilderCoverageTest, SerialCallbacks) {
  std::atomic<bool> data_received{false};
  std::atomic<bool> connected{false};
  std::atomic<bool> disconnected{false};
  std::atomic<bool> error_occurred{false};

  auto serial = unilink::serial("/dev/null", 9600)

                    .on_data([&data_received](const std::string& data) { data_received = true; })
                    .on_connect([&connected]() { connected = true; })
                    .on_disconnect([&disconnected]() { disconnected = true; })
                    .on_error([&error_occurred](const std::string& error) { error_occurred = true; })
                    .build();

  EXPECT_NE(serial, nullptr);
  serial->stop();
}

/**
 * @brief Test Serial use_independent_context
 */
TEST_F(BuilderCoverageTest, SerialIndependentContext) {
  auto serial = unilink::serial("/dev/null", 9600).use_independent_context(true).build();

  EXPECT_NE(serial, nullptr);
  serial->stop();
}

// ============================================================================
// UNIFIED BUILDER COVERAGE TESTS
// ============================================================================

/**
 * @brief Test all builders with various configurations
 */
TEST_F(BuilderCoverageTest, AllBuildersWithVariousConfigurations) {
  uint16_t port = getTestPort();

  // TCP Server with all options
  auto server = unilink::tcp_server(port)
                    .multi_client(10)

                    .auto_manage(true)
                    .use_independent_context(false)
                    .enable_port_retry(true, 3, 1000)
                    .on_data([](const std::string& data) {})
                    .on_connect([](size_t id, const std::string& ip) {})
                    .on_disconnect([](size_t id) {})
                    .on_error([](const std::string& error) {})
                    .build();
  EXPECT_NE(server, nullptr);

  // TCP Client with all options
  auto client = unilink::tcp_client("127.0.0.1", port + 1)

                    .auto_manage(true)
                    .use_independent_context(false)
                    .on_data([](const std::string& data) {})
                    .on_connect([]() {})
                    .on_disconnect([]() {})
                    .on_error([](const std::string& error) {})
                    .build();
  EXPECT_NE(client, nullptr);

  // Serial with all options
  auto serial = unilink::serial("/dev/null", 115200)

                    .auto_manage(true)
                    .use_independent_context(false)
                    .on_data([](const std::string& data) {})
                    .on_connect([]() {})
                    .on_disconnect([]() {})
                    .on_error([](const std::string& error) {})
                    .build();
  EXPECT_NE(serial, nullptr);

  server->stop();
  client->stop();
  serial->stop();
}
