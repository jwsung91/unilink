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

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>

#include "test/mocks/mock_uds_acceptor.hpp"
#include "test/mocks/mock_uds_socket.hpp"
#include "test_utils.hpp"
#include "unilink/transport/uds/uds_client.hpp"
#include "unilink/transport/uds/uds_server.hpp"
#include "unilink/wrapper/uds_client/uds_client.hpp"
#include "unilink/wrapper/uds_server/uds_server.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using namespace std::chrono_literals;

namespace unilink::wrapper {
namespace {

TEST(UdsClientWrapperAdvancedTest, AutoManageStartsInjectedTransport) {
  boost::asio::io_context ioc;
  config::UdsClientConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uwc").string();

  auto* mock_socket = new test::mocks::MockUdsSocket();
  auto transport_client =
      transport::UdsClient::create(cfg, std::unique_ptr<interface::UdsSocketInterface>(mock_socket), ioc);

  EXPECT_CALL(*mock_socket, async_connect(_, _)).WillOnce(Invoke([&ioc](const auto&, auto handler) {
    boost::asio::post(ioc, [handler]() { handler({}); });
  }));
  EXPECT_CALL(*mock_socket, async_read_some(_, _)).WillRepeatedly(Invoke([](const auto&, auto) {}));

  UdsClient client(std::static_pointer_cast<interface::Channel>(transport_client));
  client.auto_manage(true);

  ioc.restart();
  ioc.run_for(100ms);

  EXPECT_TRUE(client.is_connected());
}

TEST(UdsClientWrapperAdvancedTest, StartFutureReflectsTransportFailure) {
  boost::asio::io_context ioc;
  config::UdsClientConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uwc-fail").string();

  auto* mock_socket = new test::mocks::MockUdsSocket();
  auto transport_client =
      transport::UdsClient::create(cfg, std::unique_ptr<interface::UdsSocketInterface>(mock_socket), ioc);

  EXPECT_CALL(*mock_socket, async_connect(_, _)).WillOnce(Invoke([&ioc](const auto&, auto handler) {
    boost::asio::post(ioc, [handler]() { handler(make_error_code(boost::asio::error::connection_refused)); });
  }));

  UdsClient client(std::static_pointer_cast<interface::Channel>(transport_client));
  auto started = client.start();

  ioc.restart();
  ioc.run_for(100ms);

  ASSERT_EQ(started.wait_for(0ms), std::future_status::ready);
  EXPECT_FALSE(started.get());
}

TEST(UdsServerWrapperAdvancedTest, AutoManageStartsInjectedTransport) {
  boost::asio::io_context ioc;
  config::UdsServerConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uws").string();
  test::TestUtils::removeFileIfExists(cfg.socket_path);

  auto* mock_acceptor = new test::mocks::MockUdsAcceptor();
  auto transport_server =
      transport::UdsServer::create(cfg, std::unique_ptr<interface::UdsAcceptorInterface>(mock_acceptor), ioc);

  EXPECT_CALL(*mock_acceptor, open(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, bind(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, listen(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, async_accept(_)).WillOnce(Invoke([](auto) {}));

  UdsServer server(std::static_pointer_cast<interface::Channel>(transport_server));
  server.auto_manage(true);

  ioc.restart();
  ioc.poll();

  EXPECT_TRUE(server.is_listening());
}

TEST(UdsServerWrapperAdvancedTest, StartFutureReflectsBindFailure) {
  boost::asio::io_context ioc;
  config::UdsServerConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uws-fail").string();
  test::TestUtils::removeFileIfExists(cfg.socket_path);

  auto* mock_acceptor = new test::mocks::MockUdsAcceptor();
  auto transport_server =
      transport::UdsServer::create(cfg, std::unique_ptr<interface::UdsAcceptorInterface>(mock_acceptor), ioc);

  EXPECT_CALL(*mock_acceptor, open(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, bind(_, _)).WillOnce(Invoke([](const auto&, boost::system::error_code& ec) {
    ec = make_error_code(boost::asio::error::address_in_use);
  }));

  UdsServer server(std::static_pointer_cast<interface::Channel>(transport_server));
  auto started = server.start();

  ioc.restart();
  ioc.poll();

  ASSERT_EQ(started.wait_for(0ms), std::future_status::ready);
  EXPECT_FALSE(started.get());
  EXPECT_FALSE(server.is_listening());
}

}  // namespace
}  // namespace unilink::wrapper
