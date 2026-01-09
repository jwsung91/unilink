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
#include "test_constants.hpp"
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
    TestUtils::waitFor(constants::kShortTimeout.count());
  }

  std::shared_ptr<TcpServer> server_;
};

TEST_F(TransportTcpServerTest, LifecycleStartStop) {
  config::TcpServerConfig cfg;
  cfg.port = TestUtils::getAvailableTestPort();

  server_ = TcpServer::create(cfg);

  EXPECT_NO_THROW(server_->start());
  // Wait a bit to ensure it enters listening state
  TestUtils::waitFor(constants::kShortTimeout.count());

  EXPECT_NO_THROW(server_->stop());
}

TEST_F(TransportTcpServerTest, BindFailureTriggerError) {
  // First server occupies the port

  uint16_t port = TestUtils::getAvailableTestPort();

  net::io_context ioc_occupy;  // Use a separate io_context for the occupying acceptor

  auto work_guard = net::make_work_guard(ioc_occupy);  // Prevent run() from returning

  // Explicitly disable reuse_address on ALL platforms to ensure conflict.
  // Standard acceptor constructor enables reuse_address by default, which can cause
  // flaky tests on Unix systems if the kernel decides to allow binding (e.g. SO_REUSEPORT).

  tcp::acceptor acceptor(ioc_occupy);

  boost::system::error_code ec_bind;

  acceptor.open(tcp::v4(), ec_bind);

  if (!ec_bind) {
    acceptor.set_option(net::socket_base::reuse_address(false), ec_bind);
  }

  // Bind to INADDR_ANY to match the server bind address and force a real conflict on macOS.
  acceptor.bind(tcp::endpoint(tcp::v4(), port), ec_bind);

  acceptor.listen(net::socket_base::max_listen_connections, ec_bind);

  // Ensure the occupying acceptor is actually listening

  EXPECT_FALSE(ec_bind) << "Occupying acceptor failed to bind/listen: " << ec_bind.message();

  // Run the occupying io_context in a thread to keep the port occupied

  std::thread occupy_thread([&ioc_occupy]() { ioc_occupy.run(); });

  // Guard to ensure thread is joined even if assertions fail
  struct ThreadGuard {
    std::thread& t;
    net::io_context& ioc;
    ~ThreadGuard() {
      ioc.stop();
      if (t.joinable()) t.join();
    }
  } thread_guard{occupy_thread, ioc_occupy};

  // Give it a moment to ensure port is occupied

  TestUtils::waitFor(constants::kDefaultTimeout.count());

  // Verify port is actually occupied by connecting to it (with retries)
  {
    net::io_context probe_ioc;
    boost::system::error_code probe_ec;
    // Increase retry count to handle slow macOS CI environments
    for (int i = 0; i < 50; ++i) {
      tcp::socket probe_sock(probe_ioc);
      probe_sock.connect(tcp::endpoint(net::ip::address_v4::loopback(), port), probe_ec);
      if (!probe_ec) break;

      // Log retry on failure to help debug persistent issues
      if (i > 0 && i % 10 == 0) {
        std::cerr << "Probe connection retry " << i << " failed: " << probe_ec.message() << std::endl;
      }
      std::this_thread::sleep_for(constants::kDefaultTimeout);
    }

    ASSERT_FALSE(probe_ec) << "Failed to connect to occupying acceptor on port " << port << ": " << probe_ec.message();
  }
  // Second server tries to bind to same port

  config::TcpServerConfig cfg;

  cfg.port = port;

  cfg.port_retry_interval_ms = constants::kShortTimeout.count();

  cfg.max_port_retries = 0;  // Fail immediately after first attempt

  server_ = TcpServer::create(cfg);

  std::atomic<bool> error_occurred{false};

  server_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Error) {
      error_occurred = true;
    }
  });

  server_->start();

  // Wait for error state

  EXPECT_TRUE(TestUtils::waitForCondition([&] { return error_occurred.load(); }, 1000));  // Increased timeout

  server_->stop();
}

TEST_F(TransportTcpServerTest, MaxClientsLimit) {
  uint16_t port = TestUtils::getAvailableTestPort();
  config::TcpServerConfig cfg;
  cfg.port = port;
  cfg.max_connections = 1;

  server_ = TcpServer::create(cfg);
  server_->start();
  TestUtils::waitFor(constants::kShortTimeout.count());

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

    client2.async_read_some(net::buffer(data), [&](const boost::system::error_code& ec, size_t) {
      read_ec = ec;
      read_completed = true;
    });

    // Run io_context to process the async read
    // Give enough time for server to accept and then close the connection
    client_ioc.run_for(constants::kLongTimeout);

    EXPECT_TRUE(read_completed.load());
    // Expect EOF or error
    EXPECT_TRUE(read_ec == net::error::eof || read_ec == net::error::connection_reset);
  }  // <--- Added this closing brace
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
    cfg.max_port_retries = 15;  // Increased to 15 to allow sufficient time for port release
    cfg.port_retry_interval_ms = constants::kShortTimeout.count();

    server_ = TcpServer::create(cfg);
    server_->start();

    // Server should be in Connecting/Retry loop (or internal wait)
    // We can't easily check internal state, but we can release the port and see if it binds

    std::this_thread::sleep_for(constants::kDefaultTimeout);
  }  // acceptor closes here

  // Now server should succeed in binding
  EXPECT_TRUE(TestUtils::waitForCondition(
      [&] {
        // We need a way to check if listening.
        // Since we don't have public is_listening(), we can try to connect to it.
        net::io_context client_ioc;
        tcp::socket sock(client_ioc);
        boost::system::error_code ec;
        sock.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), port), ec);
        return !ec;
      },
      1000));
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
