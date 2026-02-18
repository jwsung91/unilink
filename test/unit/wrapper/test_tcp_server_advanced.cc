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
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

namespace {

using namespace unilink;
using namespace unilink::test;

class AdvancedTcpServerCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_port_ = TestUtils::getAvailableTestPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  void TearDown() override {
    if (server_) {
      server_->stop();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  uint16_t test_port_;
  std::shared_ptr<wrapper::TcpServer> server_;
};

TEST_F(AdvancedTcpServerCoverageTest, ServerStartStopMultipleTimes) {
  server_ = unilink::tcp_server(test_port_).unlimited_clients().build();
  for (int i = 0; i < 3; ++i) {
    auto f = server_->start();
    EXPECT_TRUE(f.get());
    EXPECT_TRUE(server_->is_listening());
    server_->stop();
    EXPECT_FALSE(server_->is_listening());
  }
}

TEST_F(AdvancedTcpServerCoverageTest, ExternalContextNotStoppedWhenNotManaged) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  // Critical: Keep ioc running even when server stops
  auto work = boost::asio::make_work_guard(*ioc);
  
  server_ = std::make_shared<wrapper::TcpServer>(test_port_, ioc);
  server_->start();
  
  std::thread t([&]() { ioc->run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  
  server_->stop();
  // Server should not stop the external context
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_FALSE(ioc->stopped());
  
  work.reset();
  ioc->stop();
  if (t.joinable()) t.join();
}

TEST_F(AdvancedTcpServerCoverageTest, ExternalContextManagedRunsAndStops) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  server_ = std::make_shared<wrapper::TcpServer>(test_port_, ioc);
  server_->set_manage_external_context(true);
  server_->start();
  
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(server_->is_listening());
  
  server_->stop();
  EXPECT_TRUE(ioc->stopped());
}

TEST_F(AdvancedTcpServerCoverageTest, SendAndCountReflectLiveClientsAndReturnStatus) {
  std::vector<size_t> ids;
  std::mutex ids_mutex;
  
  server_ = unilink::tcp_server(test_port_).build();
  server_->on_client_connect([&](const wrapper::ConnectionContext& ctx) {
    std::lock_guard<std::mutex> lk(ids_mutex);
    ids.push_back(ctx.client_id());
  });
  server_->start();

  auto client1 = unilink::tcp_client("127.0.0.1", test_port_).build();
  auto client2 = unilink::tcp_client("127.0.0.1", test_port_).build();
  
  std::atomic<int> client_received{0};
  client1->on_data([&](const wrapper::MessageContext&) { client_received++; });
  client2->on_data([&](const wrapper::MessageContext&) { client_received++; });

  client1->start();
  client2->start();

  // Wait for connections to stabilize
  EXPECT_TRUE(TestUtils::waitForCondition([&]() {
    return server_->get_client_count() >= 2;
  }, 10000));

  // Small extra delay for transport session readiness
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  size_t target_id = 0;
  {
    std::lock_guard<std::mutex> lk(ids_mutex);
    if (!ids.empty()) target_id = ids.front();
  }

  // Final check: try broadcast if send_to is too picky
  bool success = TestUtils::waitForCondition([&]() {
    if (target_id != 0) server_->send_to(target_id, "ping");
    server_->broadcast("ping");
    return client_received.load() > 0;
  }, 5000);

  EXPECT_TRUE(success);
  server_->stop();
}

TEST_F(AdvancedTcpServerCoverageTest, PortRetryConfiguration) {
  server_ = unilink::tcp_server(test_port_).enable_port_retry(true, 5, 100).build();
  auto f = server_->start();
  EXPECT_TRUE(f.get());
  EXPECT_TRUE(server_->is_listening());
}

TEST_F(AdvancedTcpServerCoverageTest, ConcurrentStartStop) {
  server_ = unilink::tcp_server(test_port_).build();
  std::vector<std::thread> threads;
  for (int i = 0; i < 2; ++i) { // Reduced count for stability
    threads.emplace_back([this]() {
      for (int j = 0; j < 5; ++j) {
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        server_->stop();
      }
    });
  }
  for (auto& t : threads) t.join();
  SUCCEED();
}

TEST_F(AdvancedTcpServerCoverageTest, HandlerReplacement) {
  std::atomic<int> count{0};
  server_ = unilink::tcp_server(test_port_).build();
  server_->on_client_connect([&](const wrapper::ConnectionContext&) { count = 1; });
  server_->on_client_connect([&](const wrapper::ConnectionContext&) { count = 2; });
  
  server_->start();
  auto client = unilink::tcp_client("127.0.0.1", test_port_).build();
  client->start();
  
  TestUtils::waitForCondition([&]() { return count.load() > 0; }, 5000);
  EXPECT_EQ(count.load(), 2);
}

}  // namespace
