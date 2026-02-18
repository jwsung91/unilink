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

class AdvancedTcpClientCoverageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_port_ = TestUtils::getAvailableTestPort();
    server_ = std::make_shared<wrapper::TcpServer>(test_port_);
    server_->start();
  }

  void TearDown() override {
    if (client_) client_->stop();
    if (server_) server_->stop();
  }

  uint16_t test_port_;
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
};

TEST_F(AdvancedTcpClientCoverageTest, ClientStartStopMultipleTimes) {
  client_ = unilink::tcp_client("127.0.0.1", test_port_).build();
  for (int i = 0; i < 3; ++i) {
    auto f = client_->start();
    EXPECT_TRUE(f.get());
    EXPECT_TRUE(client_->is_connected());
    client_->stop();
    EXPECT_FALSE(client_->is_connected());
  }
}

TEST_F(AdvancedTcpClientCoverageTest, ExternalContextNotStoppedWhenNotManaged) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  auto work = boost::asio::make_work_guard(*ioc);

  client_ = std::make_shared<wrapper::TcpClient>("127.0.0.1", test_port_, ioc);
  client_->start();

  std::thread t([&]() { ioc->run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  client_->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_FALSE(ioc->stopped());

  work.reset();
  ioc->stop();
  if (t.joinable()) t.join();
}

TEST_F(AdvancedTcpClientCoverageTest, ExternalContextManagedRunsAndStops) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  client_ = std::make_shared<wrapper::TcpClient>("127.0.0.1", test_port_, ioc);
  client_->set_manage_external_context(true);
  client_->start();

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return client_->is_connected(); }, 5000));
  client_->stop();
  EXPECT_TRUE(ioc->stopped());
}

TEST_F(AdvancedTcpClientCoverageTest, AutoManageStartsClientAndInvokesCallback) {
  std::atomic<bool> connected{false};
  client_ = unilink::tcp_client("127.0.0.1", test_port_)
                .auto_manage(true)
                .on_connect([&](const wrapper::ConnectionContext&) { connected = true; })
                .build();

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return connected.load(); }, 10000));
}

TEST_F(AdvancedTcpClientCoverageTest, SendMultipleMessages) {
  std::atomic<int> received{0};
  // Ensure handler is registered BEFORE anything starts
  server_->on_data([&](const wrapper::MessageContext&) { received++; });

  client_ = unilink::tcp_client("127.0.0.1", test_port_).build();
  client_->start();

  ASSERT_TRUE(TestUtils::waitForCondition([&]() { return client_->is_connected(); }, 5000));

  // Give a small stabilization delay
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 0; i < 5; ++i) {
    client_->send("msg");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Safe interval
  }

  EXPECT_TRUE(TestUtils::waitForCondition([&]() { return received.load() >= 5; }, 5000));
}

}  // namespace
