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
#include <memory>
#include <thread>
#include <vector>

#include "test_utils.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class TransportTcpServerTest : public ::testing::Test {
 protected:
  void TearDown() override {
    if (server_) {
      server_->stop();
      server_.reset();
    }
    // Give some time for io_context to cleanup
    TestUtils::waitFor(50);
  }

  std::shared_ptr<TcpServer> server_;
};

TEST_F(TransportTcpServerTest, LifecycleStartStop) {
  config::TcpServerConfig cfg;
  cfg.port = TestUtils::getAvailableTestPort();
  
  server_ = TcpServer::create(cfg);
  
  EXPECT_NO_THROW(server_->start());
  // Wait a bit to ensure it enters listening state
  TestUtils::waitFor(50);
  
  EXPECT_NO_THROW(server_->stop());
}

TEST_F(TransportTcpServerTest, BindFailureTriggerError) {
  // First server occupies the port
  uint16_t port = TestUtils::getAvailableTestPort();
  net::io_context ioc;
  tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), port));
  
  // Second server tries to bind to same port
  config::TcpServerConfig cfg;
  cfg.port = port;
  cfg.port_retry_interval_ms = 50;
  cfg.max_port_retries = 0; // Fail immediately after first attempt
  
  server_ = TcpServer::create(cfg);
  
  std::atomic<bool> error_occurred{false};
  server_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Error) {
      error_occurred = true;
    }
  });
  
  server_->start();
  
  // Wait for error state
  EXPECT_TRUE(TestUtils::waitForCondition([&] { return error_occurred.load(); }, 500));
  
  server_->stop();
}

TEST_F(TransportTcpServerTest, MaxClientsLimit) {
  uint16_t port = TestUtils::getAvailableTestPort();
  config::TcpServerConfig cfg;
  cfg.port = port;
  cfg.max_connections = 1;
  
  server_ = TcpServer::create(cfg);
  server_->start();
  TestUtils::waitFor(50);
  
  // Client 1 connects
  net::io_context client_ioc;
  tcp::socket client1(client_ioc);
  client1.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), port));
  
  // Client 2 connects - should be accepted but then disconnected or rejected depending on implementation
  // Unilink implementation usually accepts and then immediately closes if limit reached, 
  // or logic might handle it differently.
  // Let's verify behavior.
  
  tcp::socket client2(client_ioc);
  boost::system::error_code ec;
  client2.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), port), ec);
  
  // Check if client2 is disconnected
  if (!ec) {
    // Try to read, should get EOF if server closed it
    char data[1];
    std::atomic<bool> read_completed{false};
    boost::system::error_code read_ec;
    
    client2.async_read_some(net::buffer(data), 
      [&](const boost::system::error_code& ec, size_t) {
        read_ec = ec;
        read_completed = true;
      });
      
    // Run io_context to process the async read
    // Give enough time for server to accept and then close the connection
    client_ioc.run_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(read_completed.load());
    // Expect EOF or error
    EXPECT_TRUE(read_ec == net::error::eof || read_ec == net::error::connection_reset);
  } // <--- Added this closing brace
}

TEST_F(TransportTcpServerTest, PortBindingRetrySuccess) {
  uint16_t port = TestUtils::getAvailableTestPort();
  
  // Occupy port temporarily
  {
    net::io_context ioc;
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), port));
    
    config::TcpServerConfig cfg;
    cfg.port = port;
    cfg.enable_port_retry = true;
    cfg.max_port_retries = 3;
    cfg.port_retry_interval_ms = 50;
    
    server_ = TcpServer::create(cfg);
    server_->start();
    
    // Server should be in Connecting/Retry loop (or internal wait)
    // We can't easily check internal state, but we can release the port and see if it binds
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } // acceptor closes here
  
  // Now server should succeed in binding
  EXPECT_TRUE(TestUtils::waitForCondition([&] { 
    // We need a way to check if listening. 
    // Since we don't have public is_listening(), we can try to connect to it.
    net::io_context client_ioc;
    tcp::socket sock(client_ioc);
    boost::system::error_code ec;
    sock.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), port), ec);
    return !ec;
  }, 1000));
}

TEST_F(TransportTcpServerTest, AcceptErrorHandling) {
  // To simulate accept error, we can close the acceptor externally if possible, 
  // or use a mock. Since this is a real transport test, it's hard to force accept error
  // without mocking. However, we can test that server survives trivial errors.
  // For now, let's verify basic start/stop robustness under load which might trigger some internal paths.
  
  config::TcpServerConfig cfg;
  cfg.port = TestUtils::getAvailableTestPort();
  server_ = TcpServer::create(cfg);
  server_->start();
  
  // Just ensure it doesn't crash on immediate stop
  server_->stop();
}

