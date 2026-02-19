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
      futures.push_back(std::async(std::launch::async, [host, port, i]() {
        try {
          // Add small jitter to avoid perfect collision
          std::this_thread::sleep_for(std::chrono::milliseconds(i * 10));
          
          boost::asio::io_context ioc;
          boost::asio::ip::tcp::socket socket(ioc);
          boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
          
          boost::system::error_code ec;
          socket.open(boost::asio::ip::tcp::v4());
          socket.non_blocking(true);
          socket.connect(endpoint, ec);
          
          if (ec == boost::asio::error::would_block || ec == boost::asio::error::in_progress) {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(socket.native_handle(), &write_fds);
            timeval tv{3, 0}; // Increased to 3 seconds for CI stability
            if (select(static_cast<int>(socket.native_handle() + 1), nullptr, &write_fds, nullptr, &tv) > 0) {
              return true;
            }
          } else if (!ec) {
            return true;
          }
          return false;
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
  server_ = unilink::tcp_server(test_port).single_client().build();
  ASSERT_NE(server_, nullptr);
  ASSERT_TRUE(server_->start().get());

  auto f1 = simulateClients("127.0.0.1", test_port, 1);
  EXPECT_TRUE(f1[0].get());

  auto client_futures = simulateClients("127.0.0.1", test_port, 2);
  for (auto& f : client_futures) f.wait_for(2s);
  
  EXPECT_LE(server_->get_client_count(), 1);
}

TEST_F(ClientLimitIntegrationTest, MultiClientLimitTest) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port).multi_client(2).build();
  ASSERT_NE(server_, nullptr);
  ASSERT_TRUE(server_->start().get());

  auto client_futures = simulateClients("127.0.0.1", test_port, 4);
  for (auto& f : client_futures) f.wait_for(2s);
  
  EXPECT_LE(server_->get_client_count(), 2);
}

TEST_F(ClientLimitIntegrationTest, UnlimitedClientsTest) {
  uint16_t test_port = getTestPort();
  server_ = unilink::tcp_server(test_port).unlimited_clients().build();
  ASSERT_NE(server_, nullptr);
  ASSERT_TRUE(server_->start().get());

  auto client_futures = simulateClients("127.0.0.1", test_port, 5);
  int success_count = 0;
  for (auto& f : client_futures) if (f.get()) success_count++;
  
  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return server_->get_client_count() == 5; }, 5000));
}

TEST_F(ClientLimitIntegrationTest, ClientLimitErrorHandlingTest) {
  uint16_t test_port = getTestPort();
  // Builder now throws diagnostics::BuilderException or std::invalid_argument
  EXPECT_THROW({ server_ = unilink::tcp_server(test_port).multi_client(0).build(); }, std::invalid_argument);
}
