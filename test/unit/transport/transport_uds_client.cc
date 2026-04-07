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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <memory>
#include <thread>

#include "test/mocks/mock_uds_socket.hpp"
#include "test_constants.hpp"
#include "test_utils.hpp"
#include "unilink/transport/uds/uds_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test::mocks;
using namespace unilink::test;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;

class TransportUdsClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    unilink::diagnostics::Logger::instance().set_console_output(true);
    unilink::diagnostics::Logger::instance().set_level(unilink::diagnostics::LogLevel::DEBUG);

    auto temp_path = TestUtils::makeTempFilePath("test_uds_client.sock");
    cfg.socket_path = temp_path.string();
    TestUtils::removeFileIfExists(temp_path);

    mock_socket = new MockUdsSocket();
    auto socket_ptr = std::unique_ptr<interface::UdsSocketInterface>(mock_socket);
    // Use an external io_context from IoContextManager to match common project patterns
    client = UdsClient::create(cfg, std::move(socket_ptr), ioc);
  }

  void TearDown() override {
    if (client) {
      client->stop();
      client.reset();
    }
    TestUtils::removeFileIfExists(cfg.socket_path);
    ioc.restart();
    ioc.run_for(std::chrono::milliseconds(50));
  }

  boost::asio::io_context ioc;
  config::UdsClientConfig cfg;
  MockUdsSocket* mock_socket;
  std::shared_ptr<UdsClient> client;
};

TEST_F(TransportUdsClientTest, Construction) {
  EXPECT_NE(client, nullptr);
  EXPECT_FALSE(client->is_connected());
}

TEST_F(TransportUdsClientTest, SharedFromThisCheck) {
  // Directly test if shared_from_this() works
  EXPECT_NO_THROW({
    auto self = client->shared_from_this();
    EXPECT_EQ(self.get(), client.get());
  });
}

TEST_F(TransportUdsClientTest, SuccessfulConnection) {
  // Ensure we don't call shared_from_this until it's safe
  auto self = client->shared_from_this();

  EXPECT_CALL(*mock_socket, async_connect(_, _))
      .WillOnce(Invoke([this](const net::local::stream_protocol::endpoint&,
                              std::function<void(const boost::system::error_code&)> handler) {
        net::post(ioc, [handler]() { handler(boost::system::error_code()); });
      }));

  EXPECT_CALL(*mock_socket, async_read_some(_, _))
      .WillRepeatedly(
          Invoke([](const net::mutable_buffer&, std::function<void(const boost::system::error_code&, std::size_t)>) {
            // Do nothing
          }));

  std::atomic<bool> connected{false};
  client->on_state([&connected](base::LinkState state) {
    if (state == base::LinkState::Connected) {
      connected = true;
    }
  });

  client->start();

  // Run io_context to process handlers
  ioc.restart();
  ioc.run_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(client->is_connected());
  EXPECT_TRUE(connected);
}

TEST_F(TransportUdsClientTest, ConnectionFailure) {
  EXPECT_CALL(*mock_socket, async_connect(_, _))
      .WillOnce(Invoke([this](const net::local::stream_protocol::endpoint&,
                              std::function<void(const boost::system::error_code&)> handler) {
        net::post(ioc, [handler]() { handler(make_error_code(boost::asio::error::connection_refused)); });
      }));

  std::atomic<bool> has_error{false};
  client->on_state([&has_error](base::LinkState state) {
    if (state == base::LinkState::Error) {
      has_error = true;
    }
  });

  client->start();
  ioc.restart();
  ioc.run_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(client->is_connected());
  EXPECT_TRUE(has_error);
}

TEST_F(TransportUdsClientTest, WriteData) {
  // Setup successful connection first
  std::cout << "WriteData: setting up async_connect mock" << std::endl;
  EXPECT_CALL(*mock_socket, async_connect(_, _))
      .WillOnce(Invoke([this](const net::local::stream_protocol::endpoint&,
                              std::function<void(const boost::system::error_code&)> handler) {
        std::cout << "WriteData: async_connect called, posting handler" << std::endl;
        net::post(ioc, [handler]() {
          std::cout << "WriteData: executing connect handler" << std::endl;
          handler(boost::system::error_code());
        });
      }));

  EXPECT_CALL(*mock_socket, async_read_some(_, _))
      .WillRepeatedly(
          Invoke([](const net::mutable_buffer&, std::function<void(const boost::system::error_code&, std::size_t)>) {
            // Do nothing, just stay active
          }));

  std::cout << "WriteData: starting client" << std::endl;
  client->start();

  // Give it time to connect
  std::cout << "WriteData: waiting for connection" << std::endl;
  for (int i = 0; i < 10; ++i) {
    ioc.restart();
    ioc.run_for(std::chrono::milliseconds(20));
    if (client->is_connected()) break;
  }

  ASSERT_TRUE(client->is_connected());
  std::cout << "WriteData: client connected" << std::endl;

  std::vector<uint8_t> test_data = {1, 2, 3, 4, 5};
  std::atomic<bool> write_called{false};

  EXPECT_CALL(*mock_socket, async_write(_, _))
      .WillOnce(Invoke([&](const net::const_buffer& buffer,
                           std::function<void(const boost::system::error_code&, std::size_t)> handler) {
        std::cout << "WriteData: async_write called" << std::endl;
        EXPECT_EQ(buffer.size(), test_data.size());
        write_called = true;
        net::post(ioc, [handler, size = buffer.size()]() { handler(boost::system::error_code(), size); });
      }));

  std::cout << "WriteData: calling async_write_copy" << std::endl;
  client->async_write_copy(memory::ConstByteSpan(test_data.data(), test_data.size()));

  // Process write
  std::cout << "WriteData: polling for write" << std::endl;
  for (int i = 0; i < 10; ++i) {
    ioc.restart();
    ioc.run_for(std::chrono::milliseconds(20));
    if (write_called) break;
  }

  EXPECT_TRUE(write_called);
  std::cout << "WriteData: test finished" << std::endl;
}
