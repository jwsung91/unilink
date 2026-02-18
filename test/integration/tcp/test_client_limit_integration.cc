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
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using unilink::test::TestUtils;
using namespace std::chrono_literals;

class ClientLimitIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

  void TearDown() override {
    if (server_) {
      server_->stop();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  uint16_t getTestPort() { return TestUtils::getAvailableTestPort(); }

  std::vector<std::future<bool>> simulateClients(const std::string& host, uint16_t port, int count) {
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < count; ++i) {
      futures.push_back(std::async(std::launch::async, [host, port]() {
        try {
          boost::asio::io_context ioc;
          boost::asio::ip::tcp::socket socket(ioc);
          boost::asio::ip::tcp::resolver resolver(ioc);
          auto endpoints = resolver.resolve(host, std::to_string(port));
          boost::asio::connect(socket, endpoints);
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
          socket.close();
          return true;
        } catch (...) {
          return false;
        }
      }));
    }
    return futures;
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
};

TEST_F(ClientLimitIntegrationTest, SingleClientLimitTest) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port).single_client().enable_port_retry(true, 3, 1000).build();

  ASSERT_NE(server_, nullptr);
  if (!server_->start().get()) return;

  auto client_futures = simulateClients("127.0.0.1", test_port, 3);
  std::vector<bool> results;
  for (auto& future : client_futures) results.push_back(future.get());

  int success_count = static_cast<int>(std::count(results.begin(), results.end(), true));
  EXPECT_GE(success_count, 1);
}

TEST_F(ClientLimitIntegrationTest, MultiClientLimitTest) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port).multi_client(3).enable_port_retry(true, 3, 1000).build();

  ASSERT_NE(server_, nullptr);
  if (!server_->start().get()) return;

  auto client_futures = simulateClients("127.0.0.1", test_port, 5);
  std::vector<bool> results;
  for (auto& future : client_futures) results.push_back(future.get());

  int success_count = static_cast<int>(std::count(results.begin(), results.end(), true));
  EXPECT_GE(success_count, 3);
}

TEST_F(ClientLimitIntegrationTest, UnlimitedClientsTest) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port).unlimited_clients().enable_port_retry(true, 3, 1000).build();

  ASSERT_NE(server_, nullptr);
  if (!server_->start().get()) return;

  auto client_futures = simulateClients("127.0.0.1", test_port, 5);
  std::vector<bool> results;
  for (auto& future : client_futures) results.push_back(future.get());

  int success_count = static_cast<int>(std::count(results.begin(), results.end(), true));
  EXPECT_EQ(success_count, 5);
}

TEST_F(ClientLimitIntegrationTest, ClientLimitErrorHandlingTest) {
  uint16_t test_port = getTestPort();
  EXPECT_THROW({ server_ = unilink::tcp_server(test_port).multi_client(0).build(); }, std::invalid_argument);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
